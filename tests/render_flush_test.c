#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "heap.h"
#include "video.h"

#define SENTINEL 0xA5A5A5A5U
#define GUARD_WORDS 16U

typedef struct {
    void *mapping;
    size_t mapping_size;
    uint32_t *data;
    size_t words;
} guarded_buffer_t;

static guarded_buffer_t ram;
static unsigned allocations;
static int fail_allocation;

/* Pagina inacessivel no fim: detecta inclusive leitura alem do backbuffer. */
static guarded_buffer_t guarded_alloc(size_t bytes) {
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    size_t usable = (bytes + GUARD_WORDS * sizeof(uint32_t) + page - 1) /
                    page * page;
    size_t total = usable + 2 * page;
    void *mapping = mmap(NULL, total, PROT_NONE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(mapping != MAP_FAILED);
    assert(mprotect((char *)mapping + page, usable,
                    PROT_READ | PROT_WRITE) == 0);
    uint32_t *data = (uint32_t *)((char *)mapping + page + usable - bytes);
    assert((uintptr_t)data + bytes <= UINT32_MAX);
    for (size_t i = 0; i < GUARD_WORDS; i++)
        data[-(ptrdiff_t)i - 1] = SENTINEL;
    return (guarded_buffer_t){mapping, total, data, bytes / sizeof(uint32_t)};
}

static void check_prefix(const guarded_buffer_t *buffer) {
    for (size_t i = 0; i < GUARD_WORDS; i++)
        assert(buffer->data[-(ptrdiff_t)i - 1] == SENTINEL);
}

static void guarded_free(guarded_buffer_t *buffer) {
    check_prefix(buffer);
    assert(munmap(buffer->mapping, buffer->mapping_size) == 0);
    *buffer = (guarded_buffer_t){0};
}

/* Substitui apenas o heap; o driver e a rotina de copia sao os de producao. */
void *kmalloc(size_t bytes) {
    allocations++;
    if (fail_allocation)
        return NULL;
    assert(ram.mapping == NULL);
    ram = guarded_alloc(bytes);
    return ram.data;
}

void kfree(void *ptr) {
    assert(ptr == ram.data);
    guarded_free(&ram);
}

static uint32_t pattern(size_t index, unsigned frame) {
    return ((uint32_t)index * 2654435761U) ^ (frame * 0x1020304U);
}

static void check_frame(const guarded_buffer_t *vram, uint32_t width,
                         uint32_t height, uint32_t padding, unsigned frame) {
    size_t stride = width + padding;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t expected = pattern((size_t)y * width + x, frame);
            assert(vram->data[(size_t)y * stride + x] == expected);
            assert(ram.data[(size_t)y * width + x] == expected);
        }
        for (uint32_t x = width; x < stride; x++)
            assert(vram->data[(size_t)y * stride + x] == SENTINEL);
    }
    check_prefix(vram);
    check_prefix(&ram);
}

static void run_case(uint32_t width, uint32_t height, uint32_t padding) {
    uint32_t pitch = (width + padding) * VIDEO_BYTES_PER_PIXEL;
    guarded_buffer_t vram = guarded_alloc((size_t)pitch * height);
    for (size_t i = 0; i < vram.words; i++)
        vram.data[i] = SENTINEL;

    multiboot_info_t mbd = {
        .flags = MULTIBOOT_INFO_FRAMEBUFFER_INFO,
        .framebuffer_addr = (uintptr_t)vram.data,
        .framebuffer_pitch = pitch,
        .framebuffer_width = width,
        .framebuffer_height = height,
        .framebuffer_bpp = VIDEO_TARGET_BPP,
        .framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB,
        .color_info = {16, 8, 8, 8, 0, 8}
    };
    assert(video_init(&mbd) == VIDEO_OK);
    render_flush(); /* Sem backbuffer: nao toca a VRAM. */
    for (size_t i = 0; i < vram.words; i++)
        assert(vram.data[i] == SENTINEL);

    fail_allocation = 1;
    assert(video_init_backbuffer() == VIDEO_ERR_BACKBUFFER_ALLOC);
    render_flush();
    for (size_t i = 0; i < vram.words; i++)
        assert(vram.data[i] == SENTINEL);
    fail_allocation = 0;
    assert(video_init_backbuffer() == VIDEO_OK);
    uint32_t *src = video_get_backbuffer();
    unsigned initial_allocations = allocations;

    for (unsigned frame = 1; frame <= 2; frame++) {
        for (size_t i = 0; i < ram.words; i++)
            src[i] = pattern(i, frame);
        if (frame == 1) {
            for (size_t i = 0; i < vram.words; i++)
                assert(vram.data[i] == SENTINEL);
        }
        render_flush();
        check_frame(&vram, width, height, padding, frame);
        assert(video_get_backbuffer() == src);
        assert(video_get_framebuffer() == vram.data);
        assert(allocations == initial_allocations);
    }

    /* Desenhar altera a RAM; a VRAM muda somente no proximo flush. */
    size_t last = (size_t)(height - 1) * (width + padding) + width - 1;
    uint32_t previous = vram.data[last];
    video_put_pixel(width - 1, height - 1, 0x00123456U);
    assert(vram.data[last] == previous);
    render_flush();
    assert(vram.data[last] == 0x00123456U);

    video_clear(0x00654321U);
    render_flush();
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width + padding; x++)
            assert(vram.data[(size_t)y * (width + padding) + x] ==
                   (x < width ? 0x00654321U : SENTINEL));
    }
    assert(allocations == initial_allocations);

    video_free_backbuffer();
    vram.data[0] = SENTINEL;
    render_flush(); /* RAM ja esta desmontada: acesso causaria falha. */
    assert(vram.data[0] == SENTINEL);
    guarded_free(&vram);
    printf("OK: %ux%u, padding de %u bytes/linha\n",
           width, height, padding * VIDEO_BYTES_PER_PIXEL);
}

int main(void) {
    assert(video_get_info() == NULL);
    render_flush(); /* Antes de qualquer inicializacao. */
    run_case(1, 1, 0);
    run_case(1, 3, 1);
    run_case(7, 1, 0);
    run_case(7, 5, 3);
    run_case(1024, 768, 0);
    run_case(1024, 768, 8);
    run_case(1440, 1080, 0);
    run_case(1440, 1080, 8);
    run_case(1920, 1080, 0);
    run_case(1920, 1080, 8);
    puts("render_flush: todos os 10 cenarios passaram.");
    return 0;
}

# Tarefa 19 — Rotina `render_flush()` para copiar o backbuffer para a VRAM

Responsável pela tarefa: Mateus Luiz Martins.

Base analisada em 16/11/2026: [VG OS](https://github.com/VG-Os-Team/vg-os-system),
branch padrão `beta/terminal-grafico`, commit
`755cb3948d4210e0c49f5077dc066a57c5494f22`.
Branch local da implementação: `tarefa/19-render-flush`.

## O que o sistema já tinha

O projeto é um kernel x86 de 32 bits, escrito em C e Assembly, inicializado
pelo GRUB/Multiboot. A sequência de boot prepara vídeo, memória física,
GDT/IDT/PIC, paginação e heap antes de alocar o backbuffer e abrir o terminal.

| Parte existente | Papel na tarefa |
| --- | --- |
| `src/drivers/video.c` e `include/video.h` | Guardam endereço da VRAM, largura, altura, formato RGB e pitches. Alocam o backbuffer pelo `kmalloc`. |
| `src/drivers/gfx.c` e `Render/render.c` | Desenham pixels, retângulos, caracteres, strings e logotipo. |
| `src/ui/layout.c` | Compõe cabeçalho, rodapé, janela do terminal e rolagem da tarefa 18. |
| `kernel.c` | Processa a entrada e os comandos. Tinha a função `sincronizar_vram_provisorio()`, aguardando a tarefa 19. |

O backbuffer contém `width * height * 4` bytes na RAM. A VRAM tem
`pitch * height` bytes e pode conter espaço extra ao final de cada linha.
O driver aceita pixels RGB de 32 bits no formato `0x00RRGGBB`.

## Implementação

| Arquivo | Alteração |
| --- | --- |
| `src/drivers/video.c` | Implementação pública de `render_flush()` e auxiliar privada `video_memcpy32()`. |
| `include/video.h` | Declaração da função e documentação de seu contrato. |
| `kernel.c` | Remoção da cópia provisória e substituição das quatro chamadas pela API definitiva. |
| `tests/render_flush_test.c` | Testes nativos com buffers protegidos por páginas de guarda. |
| `Makefile` | Alvo opcional `make test-video`, separado da compilação do kernel. |

A função deve ser chamada depois de terminar o desenho do quadro:

```c
layout_draw();
/* Outros desenhos no backbuffer, como o prompt e o cursor. */
render_flush();
```

No terminal existente, ela é chamada na apresentação inicial, após alterações
da entrada/comandos e nos dois pontos de apresentação de mensagens de saída.
Isso mantém o desenho agrupado antes da apresentação.

A cópia é feita com `cld; rep movsl`. No kernel x86, cada `movsl` transfere
uma palavra de **32 bits, ou 4 bytes**, de `ESI` para `EDI`; `ECX` determina a
quantidade de palavras. `cld` define a direção crescente dos endereços.
O Assembly é `volatile` e declara os efeitos sobre memória e flags ao GCC.
Essa é a implementação local de uma cópia de 32 bits; não requer a `memcpy`
da biblioteca C de um sistema hospedeiro.

- **Pitches iguais:** uma única cópia apresenta todo o backbuffer.
- **Pitches diferentes:** cada linha transfere somente `width` pixels,
  avançando a origem e o destino pelos respectivos pitches. O padding da
  VRAM é preservado.
- **Sem inicialização ou sem double buffering:** a função retorna sem acessar
  os buffers. Também evita copiar se origem e destino forem iguais.
- **Durante o flush:** nenhuma alocação é feita, o backbuffer é preservado e
  os ponteiros dos buffers não são trocados.

Por exemplo, uma linha com 1024 pixels ocupa 4096 bytes na RAM. Se o pitch
da VRAM for 4128 bytes, a função copia 4096 e avança 4128 no destino.
Copiar indiscriminadamente `pitch * height` bytes da RAM ultrapassaria o
tamanho do backbuffer; o caminho por linhas evita esse problema.

A função usa as condições de validade já estabelecidas por `video_init()`
e `video_init_backbuffer()`. A integração atual tem um único fluxo de desenho.
Se futuramente múltiplos contextos desenharem ou liberarem o backbuffer ao
mesmo tempo, será necessário coordenar esses acessos.

## Verificação

O teste nativo compila o **driver real** com um heap substituto de teste.
As páginas de guarda detectam leituras/escritas depois dos buffers e
sentinelas verificam os limites anteriores e o padding.

```bash
make test-video
make iso
make run
```

`make test-video` requer Linux x86_64, inclusive WSL2 x86_64, e um compilador
C nativo. Ele não é compilado dentro do kernel. Para a ISO e o QEMU, use
as dependências do projeto: GCC/binutils com geração de código de 32 bits,
NASM, GRUB para BIOS, xorriso e `qemu-system-i386`.

| Geometria | Padding em bytes por linha | Resultado nativo |
| --- | --- | --- |
| 1 × 1 | 0 | Passou |
| 1 × 3 | 4 | Passou |
| 7 × 1 | 0 | Passou |
| 7 × 5 | 12 | Passou |
| 1024 × 768 | 0 e 32 | Passou nos dois casos |
| 1440 × 1080 | 0 e 32 | Passou nos dois casos |
| 1920 × 1080 | 0 e 32 | Passou nos dois casos |

Além das cópias e do padding, os cenários verificam chamada antes da
inicialização, ausência de backbuffer, falha de alocação, chamadas sucessivas,
pixel no canto inferior direito, limpeza, preservação da RAM, ausência de
alocações no flush e chamada após a liberação do backbuffer.

Os quatro arquivos ELF gerados pelo Makefile, incluindo o alias padrão,
passaram na verificação `grub-file --is-x86-multiboot`. A inspeção do objeto
ELF32 confirmou a instrução `rep movsl`.

No QEMU 10.0.13, com VGA padrão e 128 MiB de RAM, passaram **33 comparações
de quadro**, sendo 11 por opção do menu. Em cada ponto, a CPU foi pausada
para comparar os pixels da VRAM com os bytes correspondentes do backbuffer.
Foram verificados boot, digitação, `Ctrl+L`, colagem, Backspace, `help`,
`info`, `version`, rolagem, comando `clear` e desligamento por `exit`.
Também foi verificado que limpar restaura o quadro inicial e colar restaura
o quadro da linha copiada. Foram salvas capturas e hashes dos quadros.

| Opção do menu | Resolução efetiva no QEMU | Resultado |
| --- | --- | --- |
| 1920 × 1080 | 1920 × 1080 | 11 comparações passaram |
| 1440 × 1080 | 1280 × 800, fornecida pelo bootloader | 11 comparações passaram |
| 1024 × 768 | 1024 × 768 | 11 comparações passaram |

A diferença de resolução na segunda opção é um comportamento da seleção de
modo do boot existente neste ambiente. A rotina usa a geometria recebida do
Multiboot; o modo 1440 × 1080 foi validado diretamente no teste nativo.

O montador emitiu um aviso preexistente de ausência de quebra de linha no
fim de `idt/idt_asm.s`; a compilação e a linkedição concluíram normalmente.

## Como apresentar ao professor

“Minha tarefa implementa a etapa de apresentação do quadro. O sistema
desenha primeiro no backbuffer, que fica na RAM. Quando o quadro está
pronto, `render_flush()` copia os pixels para a VRAM com transferências de
32 bits. Se as linhas da VRAM tiverem bytes extras, a rotina copia linha
por linha respeitando o pitch. Integrei a função ao terminal e substituí a
rotina provisória da tarefa anterior.”

Para demonstrar, inicialize a ISO, digite `help` e `info`, use Backspace,
`Ctrl+L`, `Ctrl+C` e `Ctrl+V`, e repita comandos para provocar rolagem.
Mostre também `make test-video` para evidenciar o caso de pitch com padding,
que pode não aparecer no modo VESA escolhido pelo emulador.

A tarefa 18 já tinha uma cópia provisória: a aparência da interface é
preservada. A entrega da tarefa 19 é a API no driver, sua integração e as
verificações de cópia. Não há sincronização com VSync nem troca de páginas
de vídeo, portanto esta implementação não garante ausência de tearing.

O teclado existente continua sendo lido por polling. A implementação de
IRQ1 indicada como tarefa 20 no print é uma etapa própria.

#!/usr/bin/env python3
"""
VG OS - Font Importer & Generator
Converte fontes TrueType/OpenType (.ttf / .otf) em tabelas C de bitmap para o VG OS.

Pode escolher a fonte desejada para testar, e caso ache melhor, pode mudar no principal.

Uso:
    python3 tools/import_font.py
    python3 tools/import_font.py dejavu
    python3 tools/import_font.py cascadia
    python3 tools/import_font.py comic
    python3 tools/import_font.py jetbrains
    python3 tools/import_font.py /caminho/para/fonte.ttf
"""

import sys
import os
import argparse
import urllib.request
from PIL import Image, ImageFont, ImageDraw

PRESETS = {
    "dejavu": {
        "name": "DejaVu Sans Mono (Bold / Gordinha)",
        "url": None,
        "local_paths": [
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
            "/usr/share/fonts/dejavu/DejaVuSansMono-Bold.ttf"
        ],
        "size": 18,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "cascadia": {
        "name": "Cascadia Mono (Bold / Moderna e Larga)",
        "url": "https://github.com/google/fonts/raw/main/ofl/cascadiamono/CascadiaMono%5Bwght%5D.ttf",
        "size": 18,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "comic": {
        "name": "Comic Mono (Bold / Arredondada e Encorpada)",
        "url": "https://raw.githubusercontent.com/dtinth/comic-mono-font/master/ComicMono-Bold.ttf",
        "size": 18,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "jetbrains": {
        "name": "JetBrains Mono (Bold / Encorpada)",
        "url": "https://raw.githubusercontent.com/JetBrains/JetBrainsMono/master/fonts/ttf/JetBrainsMono-Bold.ttf",
        "size": 18,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "ubuntu": {
        "name": "Ubuntu Mono (Bold / Arredondada)",
        "url": "https://github.com/google/fonts/raw/main/ufl/ubuntumono/UbuntuMono-Bold.ttf",
        "size": 19,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "fira": {
        "name": "Fira Code (Geométrica)",
        "url": "https://github.com/google/fonts/raw/main/ofl/firacode/FiraCode%5Bwght%5D.ttf",
        "size": 17,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "roboto": {
        "name": "Roboto Mono (Bold)",
        "url": "https://github.com/google/fonts/raw/main/ofl/robotomono/RobotoMono%5Bwght%5D.ttf",
        "size": 17,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "ibm": {
        "name": "IBM Plex Mono (Medium)",
        "url": "https://github.com/google/fonts/raw/main/ofl/ibmplexmono/IBMPlexMono-Medium.ttf",
        "size": 18,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    },
    "inconsolata": {
        "name": "Inconsolata (Bold)",
        "url": "https://github.com/google/fonts/raw/main/ofl/inconsolata/static/Inconsolata-Bold.ttf",
        "size": 17,
        "offset_x": 1,
        "offset_y": 1,
        "stroke": 0,
    }
}

CHAR_MAP = {i: chr(i) for i in range(32, 127)}
# Mapeamento estendido para Português (ABNT2 / CP437)
CHAR_MAP[128] = 'Ç'
CHAR_MAP[135] = 'ç'
CHAR_MAP[130] = 'é'
CHAR_MAP[144] = 'É'
CHAR_MAP[131] = 'â'
CHAR_MAP[182] = 'Â'
CHAR_MAP[133] = 'à'
CHAR_MAP[183] = 'À'
CHAR_MAP[136] = 'ê'
CHAR_MAP[210] = 'Ê'
CHAR_MAP[147] = 'ô'
CHAR_MAP[226] = 'Ô'
CHAR_MAP[160] = 'á'
CHAR_MAP[181] = 'Á'
CHAR_MAP[161] = 'í'
CHAR_MAP[214] = 'Í'
CHAR_MAP[162] = 'ó'
CHAR_MAP[224] = 'Ó'
CHAR_MAP[163] = 'ú'
CHAR_MAP[233] = 'Ú'
CHAR_MAP[198] = 'ã'
CHAR_MAP[199] = 'Ã'
CHAR_MAP[228] = 'õ'
CHAR_MAP[229] = 'Õ'
CHAR_MAP[166] = 'ª'
CHAR_MAP[167] = 'º'
CHAR_MAP[239] = '´'

def find_font_file(source):
    # Verifica se é um preset conhecido
    src_clean = source.lower().replace(" ", "").replace("-", "")
    for key, preset in PRESETS.items():
        if key in src_clean:
            # Se tiver caminho local definido no sistema
            if preset.get("local_paths"):
                for lp in preset["local_paths"]:
                    if os.path.exists(lp):
                        return lp, preset

            cache_dir = os.path.expanduser("~/.cache/vgos-fonts")
            os.makedirs(cache_dir, exist_ok=True)
            local_file = os.path.join(cache_dir, f"{key}.ttf")
            if not os.path.exists(local_file) and preset.get("url"):
                print(f"[*] Baixando fonte '{preset['name']}'...")
                try:
                    req = urllib.request.Request(preset["url"], headers={"User-Agent": "Mozilla/5.0"})
                    with urllib.request.urlopen(req) as resp, open(local_file, "wb") as out_f:
                        out_f.write(resp.read())
                except Exception as e:
                    print(f"[-] Erro ao baixar a fonte da internet: {e}")
                    sys.exit(1)
            if os.path.exists(local_file):
                return local_file, preset

    # Caso seja um arquivo local
    if os.path.exists(source):
        return source, {"name": os.path.basename(source), "size": 18, "offset_x": 1, "offset_y": -1, "stroke": 0}

    print(f"[-] Fonte ou arquivo não encontrado: '{source}'")
    print(f"    Presets disponíveis: {', '.join(PRESETS.keys())}")
    sys.exit(1)

def main():
    if len(sys.argv) == 1:
        print("=== VG OS - Seletor de Fontes ===")
        print("Escolha uma fonte para aplicar no sistema:")
        keys = list(PRESETS.keys())
        for idx, k in enumerate(keys, 1):
            print(f"  {idx}) {PRESETS[k]['name']}")
        print(f"  {len(keys) + 1}) Digitar caminho de um arquivo .ttf local")
        print("================================")
        try:
            choice = input(f"Opção (1-{len(keys) + 1}): ").strip()
            if choice.isdigit() and 1 <= int(choice) <= len(keys):
                sys.argv.append(keys[int(choice) - 1])
            elif choice == str(len(keys) + 1):
                custom_path = input("Caminho do arquivo .ttf: ").strip()
                if custom_path:
                    sys.argv.append(custom_path)
                else:
                    sys.exit(0)
            else:
                print("Opção inválida.")
                sys.exit(1)
        except (KeyboardInterrupt, EOFError):
            print("\nOperação cancelada.")
            sys.exit(0)

    parser = argparse.ArgumentParser(description="Importador e Conversor de Fontes para VG OS")
    parser.add_argument("source", help="Nome da fonte (ex: 'dejavu', 'cascadia', 'comic', 'jetbrains') ou caminho de um arquivo .ttf/.otf")
    parser.add_argument("--size", type=int, default=None, help="Tamanho em pontos (padrão varia por preset, usualmente 17-19)")
    parser.add_argument("--offset-x", type=int, default=None, help="Deslocamento horizontal (padrão: 1)")
    parser.add_argument("--offset-y", type=int, default=None, help="Deslocamento vertical (padrão: -1)")
    parser.add_argument("--stroke", type=int, default=None, help="Espessura extra das bordas (0 ou 1, para ficar mais grossa)")
    parser.add_argument("--output", default="Render/font.h", help="Caminho do arquivo de saída (padrão: Render/font.h)")

    args = parser.parse_args()

    font_path, meta = find_font_file(args.source)
    size = args.size if args.size is not None else meta.get("size", 18)
    offset_x = args.offset_x if args.offset_x is not None else meta.get("offset_x", 1)
    offset_y = args.offset_y if args.offset_y is not None else meta.get("offset_y", 1)
    stroke = args.stroke if args.stroke is not None else meta.get("stroke", 0)

    print(f"[+] Carregando fonte: {meta['name']} (pt={size}, offset=({offset_x},{offset_y}), stroke={stroke})")
    try:
        font = ImageFont.truetype(font_path, size)
    except Exception as e:
        print(f"[-] Falha ao carregar a fonte: {e}")
        sys.exit(1)

    width = 16
    height = 24

    print(f"[+] Renderizando {len(CHAR_MAP)} glifos em matrizes {width}x{height}...")

    # Garante que o diretório de destino existe
    out_dir = os.path.dirname(args.output)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(args.output, "w", encoding="utf-8") as f:
        f.write("#ifndef FONT_H\n#define FONT_H\n\n#include <stdint.h>\n\n")
        f.write(f"/**\n * Fonte: {meta['name']}\n")
        f.write(f" * Gerado automaticamente por tools/import_font.py\n")
        f.write(f" * Tamanho do caractere: {width}x{height} pixels\n */\n\n")
        f.write("static const uint16_t font_ibm_plex[256][24] = {\n")

        for i in range(256):
            rows = [0] * height
            ch = CHAR_MAP.get(i, None)
            if ch:
                im = Image.new('1', (width, height), 0)
                draw = ImageDraw.Draw(im)
                draw.text((offset_x, offset_y), ch, font=font, fill=1, stroke_width=stroke)
                for y in range(height):
                    row_val = 0
                    for x in range(width):
                        if im.getpixel((x, y)):
                            row_val |= (1 << x)
                    rows[y] = row_val

            hex_rows = ", ".join(f"0x{r:04X}" for r in rows)
            comment = f" // 0x{i:02X} ('{ch}')" if ch else f" // 0x{i:02X}"
            comma = "," if i < 255 else ""
            f.write(f"    {{{hex_rows}}}{comma}{comment}\n")

        f.write("};\n\n")
        f.write("// Alias para compatibilidade futura\n")
        f.write("#define font_bitmap font_ibm_plex\n\n")
        f.write("#endif // FONT_H\n")

    print(f"[✔] Sucesso! Arquivo '{args.output}' gerado com sucesso.")
    print("    Para testar, rode no terminal:")
    print("    make clean && make iso && qemu-system-x86_64 -cdrom vgos.iso")

if __name__ == "__main__":
    main()

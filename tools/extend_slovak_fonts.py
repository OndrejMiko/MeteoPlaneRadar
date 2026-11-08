#!/usr/bin/env python3
"""Extend upstream sparse S/T4/T5 bitmap tables from the same face's full T3.

Only missing Slovak glyphs are added. No downloaded/system fonts are needed.
S uses the same ascent as T3; larger variants use bilinear coverage scaling.
Run after replacing ClockFonts.h with an upstream version. Safe to run twice.
"""
from pathlib import Path
import math
import re

path = Path(__file__).resolve().parents[1] / 'MeteoPlaneRadar/ClockFonts.h'
s = path.read_text()
required = set(map(ord, 'áäčďéíĺľňóôŕšťúýžÁÄČĎÉÍĹĽŇÓÔŔŠŤÚÝŽ'))

def table(name):
    bits = re.search(r'static const uint8_t '+name+r'_bits\[\] PROGMEM = \{(.*?)\n\};', s, re.S)
    glyphs = re.search(r'static const AAGlyph '+name+r'_glyphs\[\] PROGMEM = \{(.*?)\n\};', s, re.S)
    data = bytearray(int(x, 16) for x in re.findall(r'0x([\da-fA-F]+)', bits[1]))
    rows = []
    for line in glyphs[1].splitlines():
        row = re.search(r'\{([^}]+)\}', line)
        if row: rows.append([int(v.strip(), 0) for v in row[1].split(',')])
    definition = re.search(r'static const AAFont '+name+r' = \{[^\n]+, (\d+), (\d+) \};', s)
    return bits, glyphs, data, rows, definition

for face in ['INTER', 'POPPINS', 'THIN']:
    _, _, source, rows, source_def = table('FONT_'+face+'_T3')
    lookup = {r[1]: r for r in rows}
    for suffix in ['S', 'T4', 'T5']:
        name = 'FONT_'+face+'_'+suffix
        bits, glyphs, data, rows, definition = table(name)
        missing = sorted(required - {r[1] for r in rows})
        if not missing: continue
        scale = int(definition[2]) / int(source_def[2])
        for cp in missing:
            off, _, w, h, dx, dy, adv = lookup[cp]
            pixels = [(source[off+n//2] >> (0 if n%2 else 4)) & 15 for n in range(w*h)]
            nw, nh = max(1, round(w*scale)), max(1, round(h*scale))
            def at(x, y): return pixels[min(h-1,max(0,y))*w+min(w-1,max(0,x))]
            out = []
            for y in range(nh):
                for x in range(nw):
                    sx, sy = (x+.5)*w/nw-.5, (y+.5)*h/nh-.5
                    ix, iy = math.floor(sx), math.floor(sy)
                    ax, ay = sx-ix, sy-iy
                    out.append(round((1-ay)*((1-ax)*at(ix,iy)+ax*at(ix+1,iy))+ay*((1-ax)*at(ix,iy+1)+ax*at(ix+1,iy+1))))
            rows.append([len(data),cp,nw,nh,round(dx*scale),round(dy*scale),round(adv*scale)])
            if len(out)%2: out.append(0)
            data.extend((out[i]<<4)|out[i+1] for i in range(0,len(out),2))
        new_bits='\n'+''.join('  '+','.join(f'0x{v:02x}' for v in data[i:i+28])+',\n' for i in range(0,len(data),28))
        new_glyphs='\n'+''.join('  {'+str(r[0])+f',0x{r[1]:04x},'+','.join(map(str,r[2:]))+'},\n' for r in sorted(rows,key=lambda r:r[1]))
        changes=[(bits.start(1),bits.end(1),new_bits.rstrip()),(glyphs.start(1),glyphs.end(1),new_glyphs.rstrip()),(definition.start(1),definition.end(1),str(len(rows)))]
        for start,end,value in sorted(changes,reverse=True): s=s[:start]+value+s[end:]
path.write_text(s.rstrip()+"\n")

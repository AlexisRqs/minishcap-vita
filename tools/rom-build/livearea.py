"""Render indexed PNGs from the user's decoded title; stdlib only."""
import re
import struct
import zlib
from pathlib import Path

def indexed_png(path, width, height, pixels):
    palette = list(dict.fromkeys(pixels))
    if len(palette) > 256: raise ValueError('Title palette exceeds 256 colors')
    indexes = {color:i for i,color in enumerate(palette)}
    raw = b''.join(b'\0' + bytes(indexes[c] for c in pixels[y*width:(y+1)*width]) for y in range(height))
    def chunk(name, data): return struct.pack('>I',len(data))+name+data+struct.pack('>I',zlib.crc32(name+data)&0xffffffff)
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+chunk(b'PLTE',b''.join(bytes(c) for c in palette).ljust(768,b'\0'))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))

def read_ppm(path):
    raw=path.read_bytes(); header=b'P6\n240 160\n255\n'
    if not raw.startswith(header) or len(raw)!=len(header)+240*160*3:
        raise ValueError('Expected a 240x160 RGB title capture')
    raw=raw[len(header):]
    return [tuple(raw[i:i+3]) for i in range(0,len(raw),3)]

def make_assets(title_path, forest_path, destination, module):
    title=read_ppm(title_path); forest=read_ppm(forest_path)
    def scaled(pixels,w,h,x0=0,y0=0,cw=240,ch=160):
        return [pixels[(y0+y*ch//h)*240+x0+x*cw//w] for y in range(h) for x in range(w)]
    indexed_png(destination/'bg.png',840,500,scaled(forest,840,500,0,8,240,143))
    icon=scaled(forest,128,128,40,0,160,160)
    logo=scaled(title,128,69,0,0,240,130)
    icon[29*128:98*128]=logo
    indexed_png(destination/'icon0.png',128,128,icon)
    startup=[title[0]]*(280*158); logo=scaled(title,260,119,0,0,240,110)
    for y in range(119):startup[y*280+10:y*280+270]=logo[y*260:(y+1)*260]
    indexed_png(destination/'startup.png',280,158,startup)
    font=bytes(int(x,16) for x in re.findall(r'\\x([0-9a-fA-F]{2})',(module/'font8x8.inc').read_text()))
    pixels=[(16,63,55)]*(280*124); text='SETTINGS'; scale=3
    x0=(280-len(text)*8*scale)//2; y0=(124-8*scale)//2
    for i,ch in enumerate(text):
        for y in range(8):
            for x in range(8):
                if font[ord(ch)*8+y] & (0x80>>x):
                    for yy in range(scale):
                        for xx in range(scale):pixels[(y0+y*scale+yy)*280+x0+(i*8+x)*scale+xx]=(244,229,183)
    indexed_png(destination/'settings.png',280,124,pixels)

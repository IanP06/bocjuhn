import fs from 'node:fs';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
globalThis.require=createRequire(import.meta.url);
globalThis.__dirname=fileURLToPath(new URL('../../browser/particles/',import.meta.url));
const {default:create}=await import('../../browser/particles/firmware-core.mjs');
const core=await create({wasmBinary:fs.readFileSync(new URL('../../browser/particles/firmware-core.wasm',import.meta.url))});
core.HEAPU8.fill(255,core._sim_flash(),core._sim_flash()+0x100000);core._sim_boot();
core._sim_button(2,1);core._sim_button(2,0);core._sim_key(22,1);core._sim_key(22,0);
for(let m=0;m<4;m++){
 core._sim_render(2048);core._sim_encoder(3+m,1);core._sim_render(2048);
 const px=core.HEAPU16.subarray(core._sim_screen()/2,core._sim_screen()/2+57600),rgb=Buffer.alloc(57600*3);
 for(let i=0;i<57600;i++){const x=(px[i]>>8)|((px[i]&255)<<8);rgb[i*3]=((x>>11)&31)*255/31;rgb[i*3+1]=((x>>5)&63)*255/63;rgb[i*3+2]=(x&31)*255/31;}
 fs.writeFileSync('build/particles/control-'+m+'.ppm',Buffer.concat([Buffer.from('P6\n240 240\n255\n'),rgb]));
}
console.log('Four control display captures saved');

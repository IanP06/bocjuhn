import fs from 'node:fs';
import assert from 'node:assert/strict';
import {webcrypto} from 'node:crypto';
import {loadPackage} from '../../browser/fm1ota.js';
const base=new URL('../../',import.meta.url);
const meta=JSON.parse(fs.readFileSync(new URL('build/rc7/build-proof.json',base)));
const original=fs.readFileSync(new URL('build/rc7/'+meta.file,base));
let touches=0;
const AsyncFunction=Object.getPrototypeOf(async function(){}).constructor;
for(const bad of [false,true]){
 const nodes=new Map(),document={getElementById:id=>{if(!nodes.has(id))nodes.set(id,{disabled:false,checked:false,textContent:''});return nodes.get(id);}};
 const bytes=new Uint8Array(original);if(bad)bytes[100]^=1;
 const fetch=async url=>url.endsWith('manifest.json')?{json:async()=>meta}:{ok:true,arrayBuffer:async()=>bytes.buffer};
 const navigator={requestMIDIAccess:async()=>{touches++;throw Error('No hardware access');}};
 const Updater=class{constructor(){touches++;throw Error('No hardware access');}};
 const code=fs.readFileSync(new URL('browser/installer.js',base),'utf8').replace(/^import .*;\r?\n/m,'').replace(/init\(\);\s*$/,'await init();');
 await new AsyncFunction('document','fetch','crypto','navigator','Updater','loadPackage',code)(document,fetch,webcrypto,navigator,Updater,loadPackage);
 assert.equal(touches,0);assert.equal(nodes.get('install').disabled,true);assert.equal(nodes.get('connect').disabled,bad);
 assert.match(nodes.get('status').textContent,bad?/check failed/:/Package verified/);
}
console.log('RC7 installer: correct hash accepted, corrupt file refused, install gated, no MIDI access on load PASS');

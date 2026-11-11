// GPL-3.0-only. UI for the unchanged upstream TONITOR/Felucca update protocol.
import {Updater,loadPackage} from './fm1ota.js';
const $=id=>document.getElementById(id);
let pkg,updater,device,busy=false;
function refresh(){ $('connect').disabled=busy||!pkg; $('install').disabled=busy||!pkg||!device||!$('agree').checked||!$('recovery').checked; }
function say(s,d=''){ $('status').textContent=s;$('detail').textContent=d; }
async function init(){try{
  const base='../release/bocjuhn-1.0-rc9/';
  const meta=await (await fetch(base+'manifest.json',{cache:'no-store'})).json();
  const response=await fetch(base+meta.file,{cache:'no-store'});
  if(!response.ok)throw Error('Package unavailable');
  const bytes=new Uint8Array(await response.arrayBuffer());
  const sha=[...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(b=>b.toString(16).padStart(2,'0')).join('');
  if(sha!==meta.sha256||bytes.length!==meta.size)throw Error('Package hash or size mismatch');
  pkg=loadPackage(bytes);
  if(pkg.product!==meta.product)throw Error('Package identity mismatch');
  $('facts').textContent=`${meta.file}\nIdentity: ${pkg.product}\nBytes: ${bytes.length}\nSHA-256: ${sha}\nHardware validation: PENDING`;
  if(!navigator.requestMIDIAccess) { pkg=null; say('Use desktop Chrome or Edge','This browser does not expose Web MIDI.'); }
  else say('Package verified — not installed','Read the guide before beginning the hardware test.');
}catch(e){pkg=null;say('Package check failed',e.message);}refresh();}
$('connect').onclick=async()=>{busy=true;refresh();try{
  device?.link.close();device=null;
  updater=new Updater(await navigator.requestMIDIAccess({sysex:true}));
  device=await updater.find();
  say(device?'Device identity: '+device.id.text:'No FM1 found',device?'No firmware has been written.':'Check the data cable and close other MIDI applications.');
}catch(e){say('Connection failed',e.message);}finally{busy=false;refresh();}};
const guard=e=>{e.preventDefault();e.returnValue='';};
$('install').onclick=async()=>{
  if(busy||!pkg||!device||!$('agree').checked||!$('recovery').checked)return;
  if(!confirm(`Install ${pkg.product} on ${device.id.text}? This tests the RC9 changes. Do not disconnect power or USB while writing.`))return;
  busy=true;refresh();window.addEventListener('beforeunload',guard);$('progress').hidden=false;
  try{
    const id=await updater.install(device,pkg.image,pkg.product,(stage,value)=>{
      say('Update: '+stage,stage==='write'?'Writing firmware. Keep power and USB connected.':'Wait for the updater to finish.');
      if(stage==='write')$('progress').value=value;
    });
    $('progress').value=100;say('Device reports: '+id,'Now test sound, controls, saving, restart and ability to reinstall TONITOR. A successful upload alone is not full hardware validation.');
  }catch(e){say('Update stopped',e.message+' Keep this error and follow the recovery guide.');}
  finally{device=null;busy=false;window.removeEventListener('beforeunload',guard);refresh();}
};
$('agree').onchange=$('recovery').onchange=refresh;
init();

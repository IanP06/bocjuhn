const $ = id => document.getElementById(id);
const ROOT = new URL('./', import.meta.url).href;
const drumNames=['KICK','SUB','SNARE','CLAP','HAT','OPEN HAT','RIM','TOM','CLICK','ZAP','METAL','SHARD','NOISE','BLOP','TICK','CRUNCH'];
const engines = ['FM','ACID','DROPLETS','FRACT','DRUID','RYUJI','HIVE','OBRADOR'];
const buttons = ['FX','SEL','ENV','LFO','EDIT','GLO','HOME','SAVE','ARP','SEQ','PLAY','REC','OCT-','OCT+'];
const ledIds = [2,3,4,5,6,7,8,9,10,11,12,13,0,1];
const keyNodes = [], buttonNodes = [], whiteKeys = [], held = new Set();
let worker, context, output, powered = false, booting = false, master = 30, shiftLocked = false;
let lastState = [], memory = null, db, peak = 0, audioBlocks = 0;
const canvas = $('lcd'), g = canvas.getContext('2d'), zoom = $('lcdZoom').getContext('2d');
const picture = g.createImageData(240,240);
zoom.imageSmoothingEnabled = false;
const dbReady = new Promise((resolve, reject) => {
  const request = indexedDB.open('tonitor-interval-v2', 1);
  request.onupgradeneeded = () => request.result.createObjectStore('memory');
  request.onerror = () => reject(request.error);
  request.onsuccess = () => { db = request.result; const read = db.transaction('memory').objectStore('memory').get('flash'); read.onsuccess = () => { memory = read.result || null; resolve(); }; read.onerror = () => reject(read.error); };
}).catch(error => { console.warn('Memory persistence unavailable', error); });
function storeMemory(bytes) {
  memory = bytes instanceof ArrayBuffer ? bytes : bytes.slice().buffer;
  if (!db) return Promise.resolve();
  return new Promise((resolve, reject) => {
    const t = db.transaction('memory','readwrite');
    t.objectStore('memory').put(memory,'flash');
    t.oncomplete = resolve; t.onerror = () => reject(t.error);
  });
}
function send(type, ...args) { if (powered) worker.postMessage({type,args}); }
function status(text) { $('deviceStatus').textContent = text; }
function showFrame(d) {
  lastState = Array.from(d.state);
  for (let i=0;i<57600;i++) {
    const c=d.pixels[i], p=i*4;
    picture.data[p]=(c>>11)*255/31; picture.data[p+1]=((c>>5)&63)*255/63;
    picture.data[p+2]=(c&31)*255/31; picture.data[p+3]=255;
  }
  if (powered) { g.putImageData(picture,0,0); zoom.putImageData(picture,0,0); }
  buttonNodes.forEach((el,i) => el.classList.toggle('page-active',powered && !!d.leds[ledIds[i]]));
  keyNodes.forEach((el,i) => el.classList.toggle('firing',powered && !!d.leds[14+i]));
  const s=lastState, page=s[22]?'RYUJI KIT':s[48]?'CLOCK':s[49]?'ACID SETTINGS':s[40]?'PRESETS':s[3]===2?'PRISM 2/2':s[3]?'GRAINS 1/2':s[20]?(s[21]?'DELAY ':'REVERB ')+s[20]+'/2':s[4]?'LOOP SPEED':s[5]?'BALLSY':s[6]?'BRICKLAYER':s[7]?'PLAY ALONG':s[2]?'EDIT':'MAIN';
  canvas.setAttribute('aria-label','Track '+(s[0]+1)+', '+engines[s[1]]+', '+page+', '+s[8]+' loops');
  $('device').dataset.page=page; $('device').dataset.track=s[0]+1;
  $('device').dataset.engine=engines[s[1]];
  whiteKeys.forEach((n,i)=>{const el=keyNodes[n],name=s[1]===5?drumNames[i]:el.dataset.note;
    if(el.querySelector('small').textContent!==name){el.querySelector('small').textContent=name;el.setAttribute('aria-label',s[1]===5?'Pad '+(i+1)+' '+name:'Key '+name);}
  });
  if (powered) status('Track '+(s[0]+1)+' · '+engines[s[1]]+' · '+page);
  if(s[22]) $('interactionHint').textContent='RYUJI KIT · Pad '+(s[23]+1)+' · K1 PAD · K2 SOURCE · K3 TUNE ±24 · K4 LOCK · HOME: random unlocked / SHIFT+HOME: this pad · Hold EDIT to close';
  else if(s[48]) $('interactionHint').textContent='CLOCK · K1 BPM '+s[50]+' · K2 target · K3 FREE/SYNC · K4 division · LOOPS: SHIFT+K4 bars';
  else if(s[49]) $('interactionHint').textContent='ACID · K1 WAVE · K2 ACCENT · K3 SLIDE MS · K4 DRIVE';
  else if (s[40]) $('interactionHint').textContent='PRESETS · Turn PRESETS to choose · ARP or SAVE to load · Slots 1–24: editable factory sounds on RYUJI / FRACT';
  else if(s[3]===2) $('interactionHint').textContent='PRISM 2/2 · K1 PITCH '+s[16]+' st · K2 REVERSE '+s[17]+'% · K3 FREEZE '+(s[18]?'ON':'OFF')+' · K4 MIX '+s[19]+'% · SEQ: page 1';
  else if(s[20]) $('interactionHint').textContent=(s[21]?'DELAY':'REVERB')+' '+s[20]+'/2 · '+(s[20]===1?(s[21]?'K1 TIME · K2 REPEATS · K3 DAMP · K4 MIX':'K1 SIZE · K2 DECAY · K3 DAMP · K4 MIX'):(s[21]?'K1 TYPE · K2 DIVISION · K3 RHYTHM · K4 WIDTH':'K1 TYPE · K2 PREDELAY · K3 DIFFUSION · K4 WIDTH'));
  else if (s[3]) $('interactionHint').textContent='GRAINS · K1 SIZE '+s[16]+' · K2 DENSITY '+s[17]+' · K3 SCATTER '+s[18]+' · K4 FEEDBACK '+s[19];
  else if(s[4]) $('interactionHint').textContent='LOOP SPEED · Track '+s[9]+'% · Loop '+(s[11]+1)+' at '+s[10]+'% · K1 track / K2 loop / K3 speed';
  else $('interactionHint').textContent='Top six buttons: tracks · PRESETS: engine · HOME: new sound · Black key 10: EDIT (hold on RYUJI: KIT) · SHIFT + KNOB 2: GRAINS';
}
function releaseAll() {
  held.clear(); keyNodes.forEach(el=>el.classList.remove('down')); buttonNodes.forEach(el=>el.classList.remove('pressed'));
  shiftLocked=false; $('shiftLatch').setAttribute('aria-pressed','false'); send('release');
}
async function power() {
  if (booting) return;
  if (context && worker) {
    if (powered) { releaseAll(); powered=false; await context.suspend(); g.clearRect(0,0,240,240); zoom.clearRect(0,0,240,240); status('Powered off'); }
    else { powered=true; await context.resume(); send('volume',master); }
    $('power').setAttribute('aria-checked',String(powered)); return;
  }
  booting=true; $('power').disabled=true; status('Loading real firmware…');
  try {
    context=new AudioContext({sampleRate:44100,latencyHint:'interactive'});
    await context.resume();
    await dbReady;
    worker=new Worker(ROOT+'engine-worker.js?v=rc10-hybrid',{type:'module'});
    await new Promise((resolve,reject)=>{
      worker.onerror=error=>{ status('Engine error — reload to retry'); console.error(error); reject(error); };
      worker.onmessage=({data:d})=>{
        if(d.type==='ready') resolve();
        else if(d.type==='error'){status('Engine error — see browser console');console.error(d.text);reject(new Error(d.text));}
        else if(d.type==='frame') showFrame(d);
        else if(d.type==='flash') storeMemory(d.flash).catch(error=>{console.error(error);status('Memory could not be saved');});
        else if(d.type==='audio') {
          audioBlocks++; peak=0; for(const v of d.samples) peak=Math.max(peak,Math.abs(v));
          $('device').dataset.audioBlocks=audioBlocks;
          $('device').dataset.audioPeak=peak.toFixed(6);
          $('device').dataset.audioState=context.state;
          output?.port.postMessage(d,[d.samples.buffer]);
        } else if(d.type==='log') console.warn(d.text);
      };
      worker.postMessage({type:'boot',flash:memory});
    });
    await context.audioWorklet.addModule(ROOT+'audio-worklet.js');
    output=new AudioWorkletNode(context,'tonitor-output',{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2]});
    output.port.onmessage=({data})=>{if(data.type==='need')worker.postMessage({type:'render'});};
    powered=true; output.connect(context.destination); send('volume',master);
    $('power').setAttribute('aria-checked','true'); status('Ready');
  } catch(error) {
    console.error(error); status('Could not start — click Power to retry');
    worker?.terminate(); worker=null; await context?.close(); context=null;
  } finally { booting=false; $('power').disabled=false; }
}
function turn(role,delta,el) {
  if(!powered || !delta)return;
  if(role===-1){master=Math.max(0,Math.min(100,master+delta));send('volume',master);el.setAttribute('aria-valuenow',master);}
  else send('encoder',role,delta);
  const angle=Number(el.dataset.angle||0)+delta*10;
  el.dataset.angle=angle;el.style.setProperty('--angle',angle+'deg');
}
function bindPress(el,down,up) {
  el.onpointerdown=e=>{if(e.button!==0)return;e.preventDefault();el.focus();el.setPointerCapture(e.pointerId);down();};
  el.onpointerup=el.onpointercancel=()=>up();
  el.onlostpointercapture=()=>up();
  el.onkeydown=e=>{if(e.key===' '||e.key==='Enter'){e.preventDefault();e.stopPropagation();if(!e.repeat)down();}};
  el.onkeyup=e=>{if(e.key===' '||e.key==='Enter'){e.preventDefault();e.stopPropagation();up();}};
  el.onclick=e=>{if(e.detail===0 && !el.matches(':active')){down();up();}};
}
function key(n,down) {
  if(!powered)return;
  if(down){if(held.has(n))return;held.add(n);}
  else {if(!held.has(n))return;held.delete(n);}
  keyNodes[n].classList.toggle('down',down);send('key',n,down?1:0);
}
const specs=[['MASTER',-1,62,81],['SELECT',0,163,81],['PRESETS',2,62,187],['ALGORITHM',1,163,187],['KNOB1',3,598,81],['KNOB2',4,698,81],['KNOB3',5,798,81],['KNOB4',6,898,81]];
for(const [label,role,x,y] of specs){
  const el=document.createElement('div');el.className='rotary';el.tabIndex=0;el.style.left=x+'px';el.style.top=y+'px';
  el.setAttribute('role','slider');el.setAttribute('aria-label',label);el.setAttribute('aria-valuemin',role===-1?'0':'-100');el.setAttribute('aria-valuemax','100');el.setAttribute('aria-valuenow',role===-1?master:'0');
  el.innerHTML='<span class="rotary-label">'+label+'</span><span class="rotary-cap"><span class="rotary-indicator"></span></span>';
  $('hardwareControls').append(el);let lastY=0,rest=0,drag=false;
  el.onpointerdown=e=>{e.preventDefault();el.focus();el.setPointerCapture(e.pointerId);drag=true;lastY=e.clientY;rest=0;};
  el.onpointermove=e=>{if(!drag)return;rest+=(lastY-e.clientY)/Math.max(.45,$('deviceFit').clientWidth/1000);lastY=e.clientY;const d=Math.trunc(rest/7);if(d){rest-=d*7;turn(role,d,el);}};
  el.onpointerup=el.onpointercancel=el.onlostpointercapture=()=>drag=false;
  el.addEventListener('wheel',e=>{e.preventDefault();turn(role,e.deltaY<0?1:-1,el);},{passive:false});
  el.onkeydown=e=>{if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].includes(e.key)){e.preventDefault();e.stopPropagation();turn(role,['ArrowUp','ArrowRight'].includes(e.key)?1:-1,el);}};
}
buttons.forEach((label,i)=>{
  const el=document.createElement('button');el.type='button';el.className='hw-button'+(i>=12?' oct':'');
  el.style.left=(i>=12?64+(i-12)*92:589+i%6*62)+'px';el.style.top=(i>=12?273:174+Math.floor(i/6)*68)+'px';
  el.innerHTML=label==='PLAY'?'PLAY<br>STOP':label;el.setAttribute('aria-label',label==='PLAY'?'PLAY STOP':label);
  $('hardwareControls').append(el);buttonNodes.push(el);let down=false;
  bindPress(el,()=>{if(!powered||down)return;down=true;el.classList.add('pressed');send('button',i,1);},()=>{if(!down)return;down=false;el.classList.remove('pressed');send('button',i,0);});
});
const notes=['F','F#','G','G#','A','A#','B','C','C#','D','D#','E'];
const functions=['AMB','EXP','CHORD','REV','PEND','RAND','SPEED','SPEED','ADSR','EDIT','SHIFT'];
let whites=0,blacks=0;
for(let n=0;n<27;n++){
  const pitch=notes[n%12],black=pitch.includes('#'),label=pitch+(3+Math.floor((n+5)/12));
  const el=document.createElement('button');el.type='button';el.className='hw-key'+(black?' black':'');el.dataset.key=n;el.dataset.note=label;
  el.style.left=(black?64+(whites-1)*55+28:62+whites*55)+'px';el.style.top=(black?349:463)+'px';
  el.setAttribute('aria-label',black?'Black key '+(blacks+1)+' '+functions[blacks]:'Key '+label);
  el.innerHTML='<span class="key-led"></span><small>'+(black?functions[blacks++]:label)+'</small>';
  if(!black){whiteKeys.push(n);whites++;}
  keyNodes.push(el);$('hardwareKeys').append(el);bindPress(el,()=>key(n,true),()=>{if(n!==25||!shiftLocked)key(n,false);});
}
const computer=['KeyA','KeyS','KeyD','KeyF','KeyG','KeyH','KeyJ','KeyK','KeyL','Semicolon','Quote','BracketRight'];
window.addEventListener('keydown',e=>{
  if(e.repeat||e.ctrlKey||e.metaKey||e.altKey||e.target.matches('input,textarea,select'))return;
  if(e.code==='ShiftLeft'||e.code==='ShiftRight'){e.preventDefault();key(25,true);return;}
  const n=computer.indexOf(e.code);if(n>=0){e.preventDefault();key(whiteKeys[n],true);}
  else if(!e.target.closest('button,[role=slider]')&&(e.code==='Space'||e.code==='Enter')){e.preventDefault();send('button',e.code==='Space'?10:11,1);}
});
window.addEventListener('keyup',e=>{
  if(e.code==='ShiftLeft'||e.code==='ShiftRight'){if(!shiftLocked)key(25,false);return;}
  const n=computer.indexOf(e.code);if(n>=0)key(whiteKeys[n],false);
  else if(!e.target.closest('button,[role=slider]')&&(e.code==='Space'||e.code==='Enter'))send('button',e.code==='Space'?10:11,0);
});
window.addEventListener('blur',releaseAll);
$('power').onclick=power;
$('panic').onclick=()=>{releaseAll();send('panic');output?.port.postMessage({type:'clear'});};
$('shiftLatch').onclick=()=>{if(!powered)return;shiftLocked=!shiftLocked;$('shiftLatch').setAttribute('aria-pressed',shiftLocked);key(25,shiftLocked);};
$('screenZoom').onclick=()=>{$('zoomPanel').hidden=!$('zoomPanel').hidden;$('screenZoom').setAttribute('aria-pressed',!$('zoomPanel').hidden);};
$('helpToggle').onclick=()=>{$('panelHelp').hidden=!$('panelHelp').hidden;$('helpToggle').setAttribute('aria-expanded',!$('panelHelp').hidden);};
const signature='TONITOR-INTERVAL-WEB-V2\n';
$('export').onclick=async()=>{await dbReady;const flash=memory||new Uint8Array(0x100000).fill(255);const url=URL.createObjectURL(new Blob([signature,flash],{type:'application/octet-stream'}));const a=document.createElement('a');a.href=url;a.download='bocjuhn-rc10-memory.bin';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);};
$('import').onchange=async e=>{
  const file=e.target.files[0];if(!file)return;
  const bytes=new Uint8Array(await file.arrayBuffer()),header=new TextEncoder().encode(signature);
  if(bytes.length!==header.length+0x100000||!header.every((v,i)=>v===bytes[i])){status('Not an INTERVAL browser memory backup');return;}
  await dbReady;await storeMemory(bytes.slice(header.length));location.reload();
};
function fit(){const scale=Math.min(1,Math.max(.25,($('deviceApp').clientWidth-48)/1000));$('deviceFit').style.width=1000*scale+'px';$('deviceFit').style.height=620*scale+'px';$('device').style.transform='scale('+scale+')';}
window.addEventListener('resize',fit);fit();
window.tonitorDiagnostics=()=>({powered,state:[...lastState],audioBlocks,peak,sampleRate:context?.sampleRate});

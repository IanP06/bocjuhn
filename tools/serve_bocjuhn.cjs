// SPDX-License-Identifier: GPL-3.0-only
// Local simulator/installer. No device access until the browser user requests it.
const http=require('node:http'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..');
const mime={'.html':'text/html; charset=utf-8','.css':'text/css','.js':'text/javascript','.mjs':'text/javascript','.wasm':'application/wasm','.png':'image/png','.json':'application/json','.txt':'text/plain','.fwsc':'application/octet-stream'};
http.createServer((req,res)=>{
 let url;try{url=decodeURIComponent(req.url.split('?')[0]);}catch{res.writeHead(400);return res.end();}
 if(url==='/'||url==='/install'){res.writeHead(302,{'Location':url==='/'?'/browser/index.html':'/browser/install.html'});return res.end();}
 if(url.endsWith('/'))url+='index.html';
 const target=path.resolve(root,'.'+url),ext=path.extname(target);
 if(!target.startsWith(root+path.sep)||!mime[ext]||!['browser','release','docs'].includes(path.relative(root,target).split(path.sep)[0])){res.writeHead(404);return res.end();}
 fs.readFile(target,(err,data)=>{if(err){res.writeHead(404);return res.end();}res.writeHead(200,{'Content-Type':mime[ext],'Cache-Control':'no-store'});res.end(data);});
}).listen(8767,'127.0.0.1',()=>console.log('bocjuhn: http://127.0.0.1:8767 — installer /install'));

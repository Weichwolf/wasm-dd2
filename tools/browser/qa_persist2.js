// QA Area 1 (v2): proper persistence check - reopen Sound Volume after reload to read stored value.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_persist'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[],crashed=false;
const vol=(page)=>page.evaluate(()=>HEAP32[0x93fd20>>2]).catch(()=>'?');
const cfg=(page)=>rd(page,0x469158);
// enumerate MEMFS files + real fnv checksum of each
const fsDump=(page)=>page.evaluate(()=>{
  if(typeof FS==='undefined') return 'FS-undefined';
  const out={};
  const walk=(p)=>{try{for(const e of FS.readdir(p)){if(e==='.'||e==='..')continue;const fp=(p==='/'?'':p)+'/'+e;let st;try{st=FS.stat(fp);}catch(_){continue;}
    if(FS.isDir(st.mode))walk(fp); else {try{const d=FS.readFile(fp);let h=2166136261>>>0;for(let i=0;i<d.length;i++){h^=d[i];h=Math.imul(h,16777619)>>>0;}out[fp]=d.length+':'+h.toString(16);}catch(_){out[fp]='?';}}}}catch(_){}}
  walk('/'); return out;
}).catch(()=>'ERR');
async function openSoundVol(page){
  await gotoButton(page,'Configuration');
  await key(page,'Enter',900); await key(page,'ArrowRight',700);   // -> Sound Volume record
  const l=await cfg(page); await key(page,'Enter',900);            // open
  return l;
}
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);
  console.log('MEMFS at boot:', JSON.stringify(await fsDump(page)));

  console.log('=== SESSION 1: lower volume + Save Config ===');
  console.log('  sound screen label="'+await openSoundVol(page)+'"');
  const v0=await vol(page); console.log('  workvol on-open (stored default)='+v0);
  for(let i=0;i<6;i++) await key(page,'ArrowLeft',350);
  const v1=await vol(page); console.log('  workvol after lowering='+v1);
  await key(page,'Escape',800);                         // back to config submenu
  // Save Configuration record
  await key(page,'ArrowRight',700); await key(page,'ArrowRight',700);
  console.log('  save label="'+await cfg(page)+'"');
  const fsPre=JSON.stringify(await fsDump(page));
  await key(page,'Enter',1400); await key(page,'Enter',1200);
  const fsPost=JSON.stringify(await fsDump(page));
  console.log('  MEMFS after Save Config:', fsPost);
  console.log('  save changed any file='+(fsPre!==fsPost));
  await key(page,'Escape',800); await key(page,'Escape',800);

  console.log('=== RELOAD ===');
  errs=[];
  await boot(page,server);
  console.log('  reopen sound screen label="'+await openSoundVol(page)+'"');
  const vR=await vol(page); console.log('  workvol on-open after reload='+vR);
  await page.screenshot({path:`${OUT}/reload_soundvol.png`});
  await key(page,'Escape',700); await key(page,'Escape',700);

  console.log('\nRESULT: default='+v0+' lowered='+v1+' afterReload='+vR+
    ' | persisted='+(vR===v1&&v1!==v0)+' revertedToDefault='+(vR===v0)+
    ' | saveWroteFile='+(fsPre!==fsPost)+' crashed='+crashed+' errs='+JSON.stringify([...new Set(errs)]));
  await b.close(); server.close(); process.exit(crashed?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);try{server.close();}catch(_){}process.exit(1);});

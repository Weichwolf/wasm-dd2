// Diagnose Name Entry input: reach it, press Enter on a letter, trace pad bits + name buffer.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const rdmem=(page,fn)=>page.evaluate(fn).catch(e=>'ERR:'+e);
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>console.log('PAGEERR',e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  // reach Name Entry: Wrecking -> Enter (mode) -> Enter (type, Championship) -> Enter (confirm)
  await gotoButton(page,'Wrecking');
  await key(page,'Enter',900); await key(page,'Enter',900); await key(page,'Enter',1200);
  // read name buffer ptr (*0x469fd4) + a snapshot
  const nameBuf=()=>rdmem(page,()=>{const p=HEAPU32[0x469fd4>>2];let s='';for(let i=0;i<16;i++){const c=HEAPU8[p+8+i];if(!c&&i>0)break;s+=c?String.fromCharCode(c):'.';}return `ptr=0x${p.toString(16)} buf="${s}"`;});
  console.log('reached NameEntry? name:',await nameBuf());
  await page.screenshot({path:'/tmp/namediag_00.png'});
  // Now press Enter and, IN PARALLEL, poll the pad words + _pad_rdown at high frequency
  const poll=async(label)=>{
    let seen={held:0,edge:0,rdown:0};
    const t0=Date.now();
    // dispatch keydown, poll for 250ms, then keyup
    await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'Enter'})));
    while(Date.now()-t0<260){
      const s=await rdmem(page,()=>({held:HEAPU16?HEAPU16[0x754448>>1]:0, edge:HEAPU16?HEAPU16[0x75444a>>1]:0, rd:HEAPU8[0x463042]}));
      if(s&&typeof s==='object'){ seen.held|=s.held; seen.edge|=s.edge; seen.rdown|=s.rd; }
    }
    await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'Enter'})));
    await page.waitForTimeout(300);
    console.log(`  ${label}: held-max=0x${seen.held.toString(16)} edge-max=0x${seen.edge.toString(16)} pad_rdown@0x463042=${seen.rdown} | ${await nameBuf()}`);
  };
  for(let i=0;i<4;i++) await poll('Enter#'+i);
  await page.screenshot({path:'/tmp/namediag_after.png'});
  // also dump the runtime accept-key binding: which _pad bit is Enter? read keymap + which slot=0xd
  const km=await rdmem(page,()=>{let o=[];for(let v=0x46302c;v<=0x463039;v++)o.push([v.toString(16),HEAPU8[v]]);return o;});
  console.log('keymap slots:',JSON.stringify(km));
  await b.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});

// QA Area 2 (v2): linear walk of Name Entry grid + backspace/DEL + empty + max-length + commit.
const {serve,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_name'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[],crashed=false;
const tap=async(page,c,post=130)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(45);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
const key=async(page,c,post=520)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(140);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
const gc=(page)=>page.evaluate(()=>({col:(HEAPU16[0x469f34>>1]-0x30)>>4,row:Math.round((HEAPU16[0x469f36>>1]-123)/17)})).catch(()=>({col:-9,row:-9}));
const name=(page)=>page.evaluate(()=>{const p=HEAPU32[0x469fd4>>2];if(!(p>0x400000&&p<0x980000))return'PTRBAD';let s='';for(let i=0;i<16;i++){const c=HEAPU8[p+8+i];if(!c)break;s+=String.fromCharCode(c);}return s;}).catch(()=>'ERR');
async function moveTo(page,col,row){for(let i=0;i<25;i++){const c=await gc(page);if(c.row===row)break;await tap(page,c.row<row?'ArrowDown':'ArrowUp');}for(let i=0;i<25;i++){const c=await gc(page);if(c.col===col)break;await tap(page,c.col<col?'ArrowRight':'ArrowLeft');}return await gc(page);}
async function delAll(page){ // repeatedly hit DEL cell (r4c12) until buffer empty
  for(let i=0;i<20;i++){ if((await name(page)).length===0)break; await moveTo(page,12,4); await tap(page,'Enter',150);} return await name(page);}
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);
  await gotoButton(page,'Wrecking');
  await key(page,'Enter',700); await key(page,'Enter',700); await key(page,'Enter',1300);
  console.log('at Name Entry, name="'+await name(page)+'" cursor='+JSON.stringify(await gc(page)));

  // === LINEAR WALK: from home, press Enter (record char), DEL it, ArrowRight; 40 steps ===
  console.log('=== linear walk (char at each cell, advancing Right) ===');
  await moveTo(page,0,0);
  let seq=''; let cells=[];
  for(let i=0;i<44;i++){
    const cur=await gc(page); const before=await name(page);
    await tap(page,'Enter',130); const after=await name(page);
    let ch;
    if(after.length>before.length){ch=after[after.length-1]; await moveTo(page,12,4); await tap(page,'Enter',130); await moveTo(page,cur.col+1<14?cur.col+1:cur.col,cur.row);}
    else if(after.length<before.length){ch='⌫';}
    else ch='.';
    cells.push(`${cur.col},${cur.row}:${ch}`); seq+=(ch==='.'?'':ch);
    if(crashed||!await alive(page)){console.log('DIED at step'+i);break;}
    await tap(page,'ArrowRight',120);
  }
  console.log('char sequence (Right-walk):', seq);
  console.log('cells:', cells.join(' '));
  await delAll(page);
  console.log('after delAll, buffer="'+await name(page)+'"');

  // === TYPED NAME: spell "BAD" via cells (B=c1r0,A=c0r0,D=c3r0) ===
  console.log('=== type "BAD" ===');
  await delAll(page);
  for(const [c,r] of [[1,0],[0,0],[3,0]]){ await moveTo(page,c,r); await tap(page,'Enter',150);}
  console.log('  buffer="'+await name(page)+'" (expect BAD)');
  // backspace/DEL one char
  await moveTo(page,12,4); await tap(page,'Enter',150);
  console.log('  after 1 DEL="'+await name(page)+'" (expect BA)');

  // === MAX LENGTH: hammer A ===
  console.log('=== max length (spam A) ===');
  await delAll(page);
  await moveTo(page,0,0);
  let prev=-1,cap=-1;
  for(let i=0;i<20;i++){ await tap(page,'Enter',110); const n=(await name(page)).length; if(n===prev){cap=n;break;} prev=n;}
  console.log('  max name length capped at='+cap+' buffer="'+await name(page)+'"');

  // === EMPTY NAME COMMIT: DEL all, go to EX (r4c13), Enter ===
  console.log('=== empty-name commit (EX with empty buffer) ===');
  await delAll(page);
  console.log('  buffer before EX="'+await name(page)+'"');
  const scrBefore=await page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
  await moveTo(page,13,4); await tap(page,'Enter',1200);
  const scrAfter=await page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
  console.log('  scr before EX='+scrBefore+' after='+scrAfter+' (changed='+(scrBefore!==scrAfter)+') acceptedEmpty='+(scrBefore!==scrAfter));
  await page.screenshot({path:`${OUT}/afterEX.png`});

  console.log('\nRESULT area2: crashed='+crashed+' alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await b.close(); server.close(); process.exit(crashed?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);try{server.close();}catch(_){}process.exit(1);});

// Verify keyboard rebind now works (GetKeyState fix): reach rebind screen, press letters,
// confirm bindings get written (the rebind buffer @0x93fd90 / keymap changes + prompt advances).
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/kbrebind'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
const rbuf=(page)=>page.evaluate(()=>{let a=[];for(let i=0;i<6;i++)a.push(HEAPU8[0x93fd90+i]);return a;}).catch(()=>'ERR'); // rebind working keymap
let crashed=false;
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>console.log('PAGEERR',e.message.split('\n')[0]));
  page.on('crash',()=>{crashed=true;});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  await gotoButton(page,'Configuration');
  await key(page,'Enter',900); await key(page,'Enter',900); await key(page,'Enter',1200); // -> Keyboard rebind
  console.log('rebind screen; working keymap @0x93fd90 =',await rbuf(page));
  await page.screenshot({path:`${OUT}/00_left.png`});
  // bind each prompt to a letter key in sequence (Up,Down,Left,Right,... prompts)
  const seq=['KeyW','KeyS','KeyA','KeyD','KeyQ','KeyE'];
  for(let i=0;i<seq.length;i++){
    await key(page,seq[i],700);
    console.log(`  bound ${seq[i]}: keymap=${JSON.stringify(await rbuf(page))} crashed=${crashed}`);
    await page.screenshot({path:`${OUT}/${i+1}_${seq[i]}.png`});
  }
  const km=await rbuf(page);
  // success = at least one binding slot changed to a pressed VK (W=0x57,S=0x53,A=0x41,D=0x44,Q=0x51,E=0x45)
  const changed=km.some(v=>[0x57,0x53,0x41,0x44,0x51,0x45].includes(v));
  console.log(`RESULT: keymap=${JSON.stringify(km)} bindings-registered=${changed} crashed=${crashed} -> ${changed&&!crashed?'PASS':'FAIL'}`);
  await b.close(); server.close(); process.exit(changed&&!crashed?0:2);
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});

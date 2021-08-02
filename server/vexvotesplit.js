const schedule = require('node-schedule');
const EOS = require('vexaniumjs');
const request = require('request');

const CONTRACT = "***";//CONTRACT account
const BPNAME = "***";//BP account

global.votesCounter = 0;
global.votername = "";
global.checkfromid = 0;
global.maxcheckid = 0;
global.voteattime = 0;

global.checkstatu = 0;
global.scanstatu = 0;
global.partvotepayvex = 0;
global.distrrate = 0;
global.distramount = 0;
global.trxid = "";
global.lastvotetime = 0;
global.votepayvex = 0;

eosConfig1 = {
  chainId: "f9f432b1851b5c179d2091a96f593aaed50ec7466b74f89301f957a83e56ce1f",
  keyProvider: "***",//private key of CONTRACT
  httpEndpoint: 'http://209.97.162.124:8080',
  mockTransactions: () => null,
  expireInSeconds: 60,
  broadcast: true,
  debug: false,
  sign: true,
  authorization: CONTRACT + '@active'
};
global.eos1 = EOS(eosConfig1);

bpconfig= {
  chainId: "f9f432b1851b5c179d2091a96f593aaed50ec7466b74f89301f957a83e56ce1f",
  keyProvider: "***",//private key of BPNAME
  httpEndpoint: 'http://209.97.162.124:8080',
  mockTransactions: () => null,
  expireInSeconds: 60,
  broadcast: true,
  debug: false,
  sign: true,
  authorization: BPNAME
};
global.bpeos = EOS(bpconfig);

function getGlobalTable( ){
  eos1.getTableRows({"scope":CONTRACT, "code":CONTRACT, "table":"producer", "json": true,"limit": 1 }).then((res)=>{
    let rowsAll = res.rows;
    if(rowsAll.length === 0)
    {
      return setTimeout(getGlobalTable, 1000);
    } 
    checkfromid = parseInt(rowsAll[0]['checkfromid'], 10);
    maxcheckid = parseInt(rowsAll[0]['maxcheckid'], 10);
    checkstatu = parseInt(rowsAll[0]['checkStatu'], 10);
    scanstatu = parseInt(rowsAll[0]['scanStatu'], 10);
    distrrate = parseInt(rowsAll[0]['distr_rate'], 10);
    distramount = parseInt(rowsAll[0]['lastdistramount'], 10);
    lastvotetime = parseInt(rowsAll[0]['lastvotetime'], 10);

    console.log('+++++ sucess getGlobalTable  checkfromid :'+ checkfromid + ' maxcheckid :'+ maxcheckid  + ' checkstatu :'+ checkstatu  + ' scanstatu :'+ scanstatu  + ' distrrate :'+ distrrate );
    if(checkstatu == 0) 
    {
      ScanVoteRecords();
    }
    else if(checkstatu == 1)
    {
      CheckVoters();
    }
  }).catch((error)=>{
    console.info('------ getGlobalTable err: ' + JSON.stringify(error))
    return setTimeout(getGlobalTable, 1000);
  });

}

function ScanVoteRecords(){
  console.log('start ScanVoteRecords');
  var filter = 'limit=1&skip=' + checkfromid;
  console.log('get action' + filter);
  request.get(`${eosConfig1.httpEndpoint}/v1/history/get_voters/${BPNAME}/?${filter}`, (err, response, body) => {
      if (err){
          console.log(' try ScanVoteRecords! error: '  +JSON.stringify(err));
          return setTimeout(getGlobalTable, 1000);
      }
      let data;
      try{
        data = JSON.parse(body);
        voteattime = parseInt(new Date(data.voters[0].createdAt).getTime()/1000);
        votername = data.voters[0].act.data.voter;
        votesCounter = data.votesCounter;

        if((scanstatu == 1 || voteattime != lastvotetime || checkfromid > 0))
        {
          ScanVote();
        }
      }catch(err){
        console.info('------ JSON.parse : ' + JSON.stringify(body)  + " |||| " + JSON.stringify(err))
        return setTimeout(getGlobalTable, 1000);
    }
    });
}

function ScanVote(){
  let toid = checkfromid + 1;
  if(toid > maxcheckid) toid = maxcheckid;
  let action = {
    account: CONTRACT,
    name: 'scanvote',
    authorization: [{
      actor:CONTRACT,
      permission: 'active'
    }],
    data: {
      voter: votername,
      createat: voteattime,
      voteid: checkfromid,
      totalvotetime: votesCounter
    }
  }    
  let doactions = [];
  doactions.push(action);

  console.log(' try ScanVote! voter: '  + action.data.voter + ' votetime: ' + action.data.createat + ' voteid: ' + action.data.voteid + ' totalvotetime: ' + action.data.totalvotetime);
  eos1.transaction({
    actions: doactions
  }).then(result => {
    console.log('+++++ sucess ScanVote! voter: '  + action.data.voter + ' votetime: ' + action.data.createat + ' voteid: ' + action.data.voteid + ' totalvotetime: ' + action.data.totalvotetime);
    if(toid <= maxcheckid && (scanstatu == 1 || voteattime != lastvotetime)) //voteattime != lastvotetime 重复相同旧记录
    {
      return setTimeout(getGlobalTable, 100);
    } 
  }).catch(error => {
    var strerr = JSON.stringify(error);
    console.log(' ------ ScanVote error:' + strerr);
    if(strerr.indexOf("front id not finish yet") > -1)
    {
      return setTimeout(getGlobalTable, 1000);
    }
  });
}

function CheckVoters(){
  let toid = checkfromid + 1;
  if(toid > maxcheckid) toid = maxcheckid;
  let action = {
    account: CONTRACT,
    name: 'checkvoters',
    authorization: [{
      actor:CONTRACT,
      permission: 'active'
    }],
    data: {
      id: checkfromid
    }
  }    
  let doactions = [];
  doactions.push(action);

  console.log(' try CheckVoters! voter fromid: '  + action.data.id );
  eos1.transaction({
    actions: doactions
  }).then(result => {
    console.log('+++++ sucess CheckVoters! voter fromid: '  + action.data.id );
    if(toid <= maxcheckid)
    {
      return setTimeout(getGlobalTable, 100);
    }  
  }).catch(error => {
    var strerr = JSON.stringify(error);
    console.log(' ------ CheckVoters error:' + strerr);
    if(strerr.indexOf("front id not finish yet") > -1)
    {
      return setTimeout(getGlobalTable, 1000);
    }
  });
}

function claimrewards(){
  const actions = [];
  let actiontmp = {
    account: 'vexcore',
    name: 'claimrewards',
    authorization: [{
      actor: BPNAME,
      permission: 'active'
    }],
    data: {
      owner: BPNAME
    }
  }    

  let action = actiontmp;
  actions.push(action);
  let useeos = bpeos;

  useeos.transaction({
    actions: actions
  }).then(result => {

    trxid = result.transaction_id;
    getVotePayTrx();
    console.log(' ++++++++ claimrewards success! ' + BPNAME + 'time :' + new Date() + 'trxid :' + trxid);
  }).catch(error => {
    var strerr = JSON.stringify(error);
    console.log(' ------ claimrewards error:' + BPNAME + + 'time :' + new Date() + " &&  " + strerr + "\r\n");
    if(strerr.indexOf("already claimed rewards within past day") == -1)
    {
      return setTimeout(function(){claimrewards();}, 1000);
    }
  });
}

function getGlobal( ){
  eos1.getTableRows({"scope":CONTRACT, "code":CONTRACT, "table":"producer", "json": true,"limit": 1 }).then((res)=>{
    let rowsAll = res.rows;
    if(rowsAll.length === 0)
    {
      return setTimeout(getGlobalTable, 1000);
    } 
    checkfromid = parseInt(rowsAll[0]['checkfromid'], 10);
    maxcheckid = parseInt(rowsAll[0]['maxcheckid'], 10);
    checkstatu = parseInt(rowsAll[0]['checkStatu'], 10);
    scanstatu = parseInt(rowsAll[0]['scanStatu'], 10);
    distrrate = parseInt(rowsAll[0]['distr_rate'], 10);
    distramount = parseInt(rowsAll[0]['lastdistramount'], 10);
    lastvotetime = parseInt(rowsAll[0]['lastvotetime'], 10);

    partvotepayvex = (votepayvex * distrrate / 10000.00).toFixed(4);
    console.log('  getGlobal  producer vote pay VEX:' + votepayvex + ' distrrate: ' + distrrate +' part vote VEX : ' + partvotepayvex);
    if(partvotepayvex > 0)
    {
      partreward();
    }

  }).catch((error)=>{
    console.info('------ getGlobalTable err: ' + JSON.stringify(error))
    return setTimeout(getGlobal, 1000);
  });

}

function getVotePayTrx(){
  console.log('start getVotePayTrx');
  console.log('get_transaction ' + trxid);
  request.get(`${eosConfig1.httpEndpoint}/v1/history/get_transaction/${trxid}`, (err, response, body) => {
      if (err){
          console.log(' try getVotePayTrx! error: '  +JSON.stringify(err));
          return setTimeout(getVotePayTrx, 1000);
      }
      var strTrx; 
      try{
        strTrx = body;
        var indexmemo = strTrx.indexOf(" VEX\",\"memo\":\"producer vote pay\"");
        if(indexmemo == -1)
        {
          console.log(' try getVotePayTrx! error: not find producer vote pay memo' );
          return setTimeout(getVotePayTrx, 1000);
        }
        else
        {
          var memo1 = strTrx.substring(0,indexmemo+1);

          var indexvex  = memo1.lastIndexOf(":\"");
          var memo3 = memo1.substring(indexvex  + 2);

          votepayvex = parseFloat(memo3);
          console.log('  getVotePayTrx  producer vote pay VEX:' + votepayvex );
          
          if(distrrate ==0)
          {
            getGlobal();
          }
          else
          {
            partvotepayvex = (votepayvex * distrrate / 10000.00).toFixed(4);
            console.log('  getGlobal  producer vote pay VEX:' + votepayvex + ' distrrate: ' + distrrate +' part vote VEX : ' + partvotepayvex);
            if(partvotepayvex > 0)
            {
              partreward();
            }
          }
        }
      }catch(err){
        console.info('------ JSON.parse : ' + JSON.stringify(body)  + " |||| " + JSON.stringify(err))
        return setTimeout(getVotePayTrx, 1000);
    }
    });
}

function partreward(){
  console.info('try partreward : ' + partvotepayvex );

  const actions = [];
  let actiontmp = {
    account: 'vex.token',
    name: 'transfer',
    authorization: [{
        actor:BPNAME,
        permission: 'active'
    }],
    data: {
        from: BPNAME,
        to: CONTRACT,
        quantity: partvotepayvex + ' VEX',
        memo: 'part vote pay for voters'
    }
  }    

  let action = actiontmp;
  actions.push(action);
  let useeos = bpeos;

  useeos.transaction({
    actions: actions
  }).then(result => {
    console.log(' ++++++++ partreward success! ' + BPNAME + 'time :' + new Date() );

    getGlobalTable();
  }).catch(error => {

    var strerr = JSON.stringify(error);
    console.log(' ++++++++ partreward error: ' + BPNAME + + 'time :' + new Date() + " &&  "+ strerr + "\r\n");
    if(strerr.indexOf("Last distr not far from") == -1 && strerr.indexOf("last check not finish") == -1)
    {
      return setTimeout(function(){partreward();}, 1000);
    }
  });
}


//4pm beijing
const  schedule1 = ()=>{
  schedule.scheduleJob('0 0 8 * * *',function(){ 
    claimrewards(); 
  });
}
schedule1();

setInterval(getGlobalTable,300000);

process.on('uncaughtException', (err) => {
  console.log(err);
  console.log(err.stack);
});

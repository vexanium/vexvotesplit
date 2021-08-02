# vexvotesplit

Auto split VEX Bp vote reward for Voters

<H2>Step 1 Set and Init contract</h2>
<pre>$ ./cleos set contract CONTRACT ./vexvotesplit

$ ./cleos push action CONTRACT init '[distr_rate, lastvotetime, voterecordnum, BPNAME]' -p CONTRACT </pre>


distr_rate: 0 - 10000 for 0% - 100%
lastvotetime and voterecordnum get from here:

http://209.97.162.124:8080/v1/history/get_voters/BPNAME/?limit=1
{
  "voters": [
    {
      "_id": "b85eba2ec96ad50601d56105",
      ****
      "createdAt": "2021-07-31T21:01:34.013Z" //lastvotetime switch this to timestamp
    }
  ],
  "votesCounter": 32 //voterecordnum
}


<H2>Step 2 Set and run server</h2>
<pre>set keys、 accounts of CONTRACT and BPNAME in vexvotesplit.js 

$ nohup node vexvotesplit.js > vexvotesplit.log 2>&1 &</pre>




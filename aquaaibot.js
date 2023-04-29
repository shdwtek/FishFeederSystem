const tmi = require('tmi.js');
const http = require('http');
const generator = require('./textGenerator');
const os = require('node:os');
const reputation = {};

function doRequest(url) {
  return new Promise(function (resolve, reject) {
    http.get(url, function(response) {
      var data = '';
      response.on('data', function (chunk) {
          data += chunk;
      });
      response.on('end', async function () {
        resolve(data);
      });
      response.on('error', async function (error) {
        reject(error);
      });
    });
  });
}

const client = new tmi.Client({
  options: { debug: true },
  connection: {
    secure: true,
    reconnect: true
  },
  identity: {
    username: process.env.TWITCH_BOT_USERNAME,
    password: process.env.TWITCH_OAUTH_TOKEN
  },
  channels: process.env.TWITCH_CHANNELS_TO_JOIN.split(',')
});

client.connect();

const eightHoursInMilliseconds = 8 * 60 * 60 * 1000; // convert 6 hours to milliseconds
var lastFeedTime = 0;

client.on('message', (channel, tags, message, self, understate) => {

  const args = message.slice(1).split(' ');
  const command = args.shift().toLowerCase();

  // Handle !feedfish
  if (command === 'feedfish') {
    const currentTime = new Date().getTime();
    if (currentTime - lastFeedTime < eightHoursInMilliseconds) {
      client.say(channel, `@${tags.username} You can only feed the fish once every 8 hours!`);
      return;
    }
    const request = http.request({
      hostname: '192.168.0.203', // Replace with correct hostname IP
      port: 8082, // Replace with correct Port
      path: '/H',
      method: 'GET'
    }, (response) => {
      client.say(channel, `@${tags.username} Thanks for feeding the fish!`);
      lastFeedTime = currentTime; // Update the last feed time
      console.log(`Response: ${response.statusCode}`);
    });
    request.on('error', (error) => {
      client.say(channel, `@${tags.username} There was an error reaching the feeder.`);
      console.error(error);
    });
    request.end();
    return;
  }

  // Handle any message containing 'aquaaibot' (case-insensitive)
  if(message.match(new RegExp(process.env.TWITCH_BOT_USERNAME, 'i'))) {
    (async () => {
      var tankData = await doRequest('http://shdwtek.net/temp.html'); // Replace URL with correct address
      tankData = tankData.replace(/<[^>]+>/g, ' ').trim().replace(/ +/, ' ').split(' ');

      var time = os.uptime();
      var day = parseInt(time / 86400);
      var promptText = process.env.OPENAI_PROMPT
        .replace(/\{botname\}/g, tags['display-name'])
        .replace('{message}', args.join(' ')).trim()
        .replace('{temp}', tankData[0] + '°F')
        .replace('{tds}', tankData[1] + 'PPM')
        .replace('{tss}', tankData[2] + 'PPM')
        .replace('{level}', tankData[3])
        .replace('{uptime}', day + 'days');

      // Add a period if necessary so the bot doesn't try to complete the prompt.
      if (!['.','?'].includes(promptText.slice(-1))) {
        promptText = `${promptText}.}`;
      }
      client.say(channel, `@${tags.username}, ${await generator.generate(promptText)}`);
    })();
    return;
  }

});

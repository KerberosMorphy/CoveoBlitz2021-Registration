const http = require('http');
const { argv } = require("yargs");
const parse = require('fast-json-parse');
const FastJson = require('fast-json');
const fastJson = require('fast-json-stringify');

const fastoJson = new FastJson();

const stringify = fastJson({
  title: 'List',
  type: 'array',
  items: {
    type: 'number',
  }
});

const PORT = argv.port || argv.p || 27178;

http.createServer((req, res) => {
  if (req.method === 'POST') {
    let track = [];
    let items = [];
    req.on('data', chunk => {
      fastoJson.on('track', (value) => {
        track = parse(value).value;
      });
      fastoJson.on('items', (value) => {
        items = parse(value).value;
      });
      fastoJson.write(chunk.toString());
    }).on('end', () => {
      const track_cumsum = [0];
      let sums = 0;
      let index = 0;
      track.forEach((t) => {
        sums += t;
        index += 1;
        track_cumsum[index] = sums;
      });
      res.end(stringify(items.map((i) => Math.abs(track_cumsum[i[1]] - track_cumsum[i[0]]))));
    });
  } else {
    res.end('ok');
  }
}).listen(PORT);

FROM node:20.20.2-alpine@sha256:fb4cd12c85ee03686f6af5362a0b0d56d50c58a04632e6c0fb8363f609372293

COPY package.json package-lock.json index.js /
RUN npm install --production

CMD ["node", "index.js"]

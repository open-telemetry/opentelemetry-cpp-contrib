FROM node:22.23.5-alpine@sha256:1314fb30ec37592d5845e5a160d522c0e680972aa534549940b8d73bde88481a

COPY package.json package-lock.json index.js /
RUN npm install --production

CMD ["node", "index.js"]

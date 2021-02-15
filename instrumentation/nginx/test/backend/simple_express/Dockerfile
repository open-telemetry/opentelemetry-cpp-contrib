FROM node:14-alpine

COPY package.json package-lock.json index.js /
RUN npm install --production

CMD ["node", "index.js"]

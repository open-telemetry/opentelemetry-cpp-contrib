FROM node:24.22.3-alpine@sha256:116b800c3754b440ee0aa3ed2f20f5bf3d99a554bfd9adeea09ea8017f6e85f5

COPY package.json package-lock.json index.js /
RUN npm install --production

CMD ["node", "index.js"]

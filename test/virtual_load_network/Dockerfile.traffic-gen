FROM alpine:3.20

RUN apk add --no-cache curl bash

COPY ./scripts/traffic-gen.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT [ "/entrypoint.sh" ]

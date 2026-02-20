# our base image
FROM ubuntu:latest
RUN apt-get update -y && apt-get install                            nginx wget curl zip openssl apt-transport-https -y && apt-get clean -y
RUN echo "daemon off;" >> /etc/nginx/nginx.conf
COPY index.html /var/www/html/
EXPOSE 80 443
CMD ["nginx"]
#docker build . -t my_nginx
#docker images
#docker run --rm -p 80:80 -i my_nginx
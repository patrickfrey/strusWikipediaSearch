#!/bin/sh

# NGINX:
cp client/*.php /usr/share/nginx/html/strus/
cp client/*.* /usr/share/nginx/html/strus/
sudo chown www-data:www-data /usr/share/nginx/html/strus/*
sudo service php5-fpm restart
sudo service nginx restart

# APACHE:
cp client/*.php /var/www/html/strus/
cp client/*.* /var/www/html/strus/

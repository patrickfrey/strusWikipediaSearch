#!/bin/sh

sudo cp client/*.php /usr/share/nginx/html/strus/
sudo cp client/*.css /usr/share/nginx/html/strus/
sudo chown www-data:www-data /usr/share/nginx/html/strus/*
sudo service php5-fpm restart
sudo service nginx restart

#!/bin/sh

sudo cp client/*.php /var/www/html/strus/
sudo cp client/*.css /var/www/html/strus/

sudo service apache2 restart

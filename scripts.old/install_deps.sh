#!/bin/sh

apt-get install python-tornado
apt-get install python-pip
pip install tornado.tcpclient

wget https://pypi.python.org/packages/source/t/tornado/tornado-4.3.tar.gz#md5=d13a99dc0b60ba69f5f8ec1235e5b232
tar xvzf tornado-4.3.tar.gz
cd tornado-4.3
python setup.py build
python setup.py install


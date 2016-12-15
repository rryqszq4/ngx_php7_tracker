ngx_php7_tracker
================

[ngx_php7_tracker](https://github.com/rryqszq4/ngx_php7_tracker) - Track php7 script language for nginx-module.   
QQ group：558795330

Requirement
-----------
- PHP 7.*.*  
- nginx-1.6.3 

Installation
------------
```sh
$ git clone https://github.com/rryqszq4/ngx_php7_tracker.git

$ wget 'http://nginx.org/download/nginx-1.6.3.tar.gz'
$ tar -zxvf nginx-1.6.3.tar.gz
$ cd nginx-1.6.3

$ export PHP_BIN=/path/to/php/bin
$ export PHP_INC=/path/to/php/include/php
$ export PHP_LIB=/path/to/php/lib

$ ./configure --user=www --group=www \
              --prefix=/path/to/nginx \
              --with-ld-opt="-Wl,-rpath,$PHP_LIB" \
              --add-module=/path/to/ngx_php7_tracker

$ make
$ make install
```

Synopsis
--------

```nginx
user www www;
worker_processes  4;

events {
    worker_connections  1024;
}

http {
    include       mime.types;
    default_type  application/octet-stream;

    keepalive_timeout  65;
    
    client_max_body_size 10m;   
    client_body_buffer_size 4096k;

    php_ini_path /usr/local/php/etc/php.ini;

    server {
        listen       80;
        server_name  localhost;
    
        location /content_by_php {
            content_by_php '
                header("Content-Type: text/html;charset=UTF-8");
                require_once("/home/www/yaf/cg/output/Sample/index.php");
            ';
        }

        location /opcode_by_php {
            opcode_by_php '
                header("Content-Type: text/html;charset=UTF-8");
                require_once("/home/www/yaf/cg/output/Sample/index.php");
            ';
        }

        location /stack_by_php {
            stack_by_php '
                header("Content-Type: text/html;charset=UTF-8");
                require_once("/home/www/yaf/cg/output/Sample/index.php");
            ';
        }
    }
}
```

Directives
----------
* [content_by_php](#content_by_php)
* [opcode_by_php](#opcode_by_php)
* [stack_by_php](#stack_by_php)

content_by_php
--------------
```sh
$ curl http://localhost/content_by_php
Hello World! I am Stranger
```

opcode_by_php
--------------
```sh
$ curl http://localhost/opcoce_by_php
```
![image](https://github.com/rryqszq4/ngx_php7_tracker/blob/master/doc/opcode_yaf.png)

stack_by_php
--------------
```sh
$ curl http://localhost/stack_by_php
```
![image](https://github.com/rryqszq4/ngx_php7_tracker/blob/master/doc/stack_yaf.png)

Copyright and License
---------------------
BSD 3-Clause License

Copyright (c) 2016, rryqszq4 <ngxphp@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

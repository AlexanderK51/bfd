# Настройка rsyslog
Настройка проводится для конфигурации: один централизованный сервер,
 который собирает логи с клиентов - виртуальных машин, на которых
  запущены сервисы VNF.
Пример написан для сервиса с названием `myapp`

## Настройка rsyslog на клиенте
Открываем конфиг rsyslog:
```sh
sudo nano /etc/rsyslog.d/myapp.conf
```

Добавляем:
```conf
module(load="imfile")

input(type="imfile"
      File="/var/log/myapp.log"
      Tag="myapp"
      Severity="info"
      Facility="local0")

*.* action(type="omfwd" target="logserver.local" port="514" protocol="tcp")
```

Перезапускаем rsyslog:
```sh
sudo systemctl restart rsyslog
```

## Настройка rsyslog на централизованном сервере
Открываем конфиг:
```sh
sudo nano /etc/rsyslog.d/central.conf
```

Добавляем:
```conf
module(load="imtcp")
input(type="imtcp" port="514")

if $programname == 'myapp' then /var/log/myapp-centralized.log
```

Перезапускаем rsyslog:
```sh
sudo systemctl restart rsyslog
```
<!--  -->
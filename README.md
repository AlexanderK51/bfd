1. mainbfd.cpp  
    bfd.hpp
    bfd-dbus-server.hpp  
    Это программа bfd процесса  

2. bfd-dbus-client.cpp  
    bfd-dbus-client.hpp
    Это тестовый клиент!
  
  
  
##BFD + UNIXSOCKET + DBSUB  
  
  #DBUS  
mkdir dbus  
cd dbus  
git clone https://github.com/Kistler-Group/sdbus-cpp.git  
//see install from github
sudo apt install libsystemd-dev  
  

  #sdbus ctrl  
busctl --user tree org.sdbuscpp.bfd
busctl --user


#XML DBUS code gen  
sudo apt install libsdbus-c++-bin


##  
busctl --user introspect org.sdbuscpp.bfd /org/sdbuscpp/bfd  
busctl --user call org.sdbuscpp.bfd /org/sdbuscpp/bfd org.sdbuscpp.Bfd set ss bfd TRUE  
busctl --user call org.sdbuscpp.bfd /org/sdbuscpp/bfd org.sdbuscpp.Bfd set ss bfd FALSE  
  

###Service - ПЕРЕХОД В РЕЖИМ СЕРВИСА  
/etc/systemd/system  
  

su - root

cat << EOF > /etc/systemd/system/bfd.service  
[Unit]  
Description=Bfdv2.0  
Documentation=http://192.10.70.50/a.kocur/bfd  
  
[Service]  
ExecStart=/home/smuser/newbfd/bfd/build/bfd 
  
[Install]  
WantedBy=multi-user.target  
EOF  
  
sudo nano /etc/dbus-1/system.d/net.msystems.bfd.conf

<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="root">
    <allow own="net.msystems.bfd"/>
  </policy>
  <policy context="default">
    <allow send_destination="net.msystems.bfd"/>
    <allow send_interface="net.msystems.Bfd"/>
    <allow receive_sender="net.msystems.bfd"/>
  </policy>
</busconfig>
 

sudo systemctl start bfd.service  
sudo systemctl status bfd.service 

sudo busctl introspect org.sdbuscpp.bfd /org/sdbuscpp/bfd  
sudo busctl call org.sdbuscpp.bfd /org/sdbuscpp/bfd org.sdbuscpp.Bfd set ss bfd TRUE  
sudo busctl call org.sdbuscpp.bfd /org/sdbuscpp/bfd org.sdbuscpp.Bfd set ss bfd FALSE  


cat << EOF > /etc/systemd/system/bfd-dbus-client.service  
[Unit]  
Description=test  
Documentation=http://192.10.70.50/a.kocur/bfd  
  
[Service]  
ExecStart=/home/smuser/newbfd/bfd/build/bfd-dbus-client  
  
[Install]  
WantedBy=multi-user.target  
EOF

sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd lst ss bfd %

sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd lst ss bfdsession %

sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd dsp ss bfdsession %
sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd dsp ss bfdsession bfd_radius

sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd mod sas bfdsession 2 bfd_radius DOWN
sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd mod sas bfdsession 2 bfd_radius UP

sudo busctl call net.msystems.bfd /net/msystems/bfd net.msystems.Bfd set ss bfdloglevel debug


sudo busctl monitor --match 'sender=net.msystems.bfd,path=/net/msystems/bfd,interface=net.msystems.Bfd,type=signal'


#ver2.1 add logger
sudo systemctl stop ipr; sudo systemctl stop bfd;
sudo systemctl start bfd; sudo systemctl start ipr;
sudo systemctl status ipr; sudo systemctl status bfd;


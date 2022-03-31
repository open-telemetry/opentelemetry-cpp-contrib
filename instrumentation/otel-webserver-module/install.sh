cd /otel-webserver-module
./gradlew assembleApacheModule -PbuildType=debug

#Changing the httpd.conf and Adding Opentelemetry.conf
cd build
tar -xf opentelemetry-webserver-sdk-x64-linux.tgz
mv opentelemetry-webserver-sdk /opt/
cd ../
cp opentelemetry_module.conf /etc/httpd/conf.d/

# Installing

cd /opt/opentelemetry-webserver-sdk
./install.sh


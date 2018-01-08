Vagrant.configure("2") do |config|
  config.vm.define "reliableUDPServer" do |reliableUDPServer|
    reliableUDPServer.vm.box = "ubuntu/trusty64"
    reliableUDPServer.vm.hostname = 'reliableUDPServer'
    reliableUDPServer.vm.box_url = "ubuntu/trusty64"

    reliableUDPServer.vm.network :private_network, ip: "10.0.0.25"
    reliableUDPServer.vm.network :forwarded_port, guest: 1050, host: 1300, protocol: "udp"

    reliableUDPServer.vm.provider :virtualbox do |v|
      v.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]
      v.customize ["modifyvm", :id, "--memory", 1024]
      v.customize ["modifyvm", :id, "--name", "reliableUDPServer"]
    end
  end

  config.vm.define "reliableUDPClient" do |reliableUDPClient|
    reliableUDPClient.vm.box = "ubuntu/trusty64"
    reliableUDPClient.vm.hostname = 'reliableUDPClient'
    reliableUDPClient.vm.box_url = "ubuntu/trusty64"

    reliableUDPClient.vm.network :private_network, ip: "10.0.0.26"
    reliableUDPClient.vm.network :forwarded_port, guest: 1050, host: 1301, protocol: "udp"

    reliableUDPClient.vm.provider :virtualbox do |v|
      v.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]
      v.customize ["modifyvm", :id, "--memory", 1024]
      v.customize ["modifyvm", :id, "--name", "reliableUDPClient"]
    end
  end
  
  config.vm.provision "shell", inline: <<-SHELL
      apt-get update
      apt-get install -y g++
    SHELL
end
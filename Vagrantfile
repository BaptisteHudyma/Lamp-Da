# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://vagrantcloud.com/search.
  config.vm.box = "generic/debian12"
  config.vm.box_version = "4.3.12"
  config.vm.define "lampbuild"
  config.vm.hostname = "lampbuild"
  config.vm.disk :disk, size: "10GB", primary: true

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # NOTE: This will enable public access to the opened port
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine and only allow access
  # via 127.0.0.1 to disable public access
  # config.vm.network "forwarded_port", guest: 80, host: 8080, host_ip: "127.0.0.1"

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network", :dev => "wlan0"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  config.vm.synced_folder "./src", "/LampColorControler/src"
  config.vm.synced_folder "./depends", "/LampColorControler/depends"
  config.vm.synced_folder "./flashInfo", "/LampColorControler/flashInfo"
  config.vm.synced_folder "./scripts", "/LampColorControler/scripts"
  config.vm.synced_folder "./simulator", "/LampColorControler/simulator"
  config.vm.synced_folder "./tools", "/LampColorControler/tools"

  config.vm.synced_folder ".", "/LampColorControler", type: "rsync",
    rsync__auto: false,
    rsync__exclude: [".vagrant", "_build", "_vagrant_build", "venv",
                     "docs", "src", "depends", "flashInfo", "scripts", "simulator", "tools",
                     "objects", "electrical", "Medias"]

  # Disable the default share of the current code directory. Doing this
  # provides improved isolation between the vagrant box and your host
  # by making sure your Vagrantfile isn't accessible to the vagrant box.
  # If you use this you may want to enable additional shared subfolders as
  # shown above.
  config.vm.synced_folder ".", "/vagrant", disabled: true

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  # config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
  #
  #   # Customize the amount of memory on the VM:
  #   vb.memory = "1024"
  # end
  #
  # View the documentation for the provider you are using for more
  # information on available options.

  # Enable provisioning with a shell script. Additional provisioners such as
  # Ansible, Chef, Docker, Puppet and Salt are also available. Please see the
  # documentation for more information about their specific syntax and use.
  config.vm.provision "shell", inline: <<-SHELL
    #
    # compilers & tools
    apt-get update -y
    apt-get install -y build-essential gcc g++ clang clang-format make cmake git
    #
    # for python dependencies (!! python is python3 !!)
    apt-get install -y python3 python3-venv python-is-python3
    #
    # all dependencies required for SFML 3.0.1
    apt-get install -y libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libfreetype-dev libvorbis-dev libflac-dev
    apt-get install -y libsfml-dev # (this sets up SFML 2.6.2)
    #
    # by default, download+install arduino-cli in _build/arduino-cli directory
    chown -R vagrant:vagrant /LampColorControler/*
    su vagrant -c "cd /LampColorControler && git submodule update --init"
    su vagrant -c "cd /LampColorControler && make mr_proper arduino-cli-download safe-install"
    #
    # for example, build indexable & simulator:
    # su vagrant -c "cd /LampColorControler && make indexable simulator"

  SHELL
end

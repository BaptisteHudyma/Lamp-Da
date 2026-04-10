
# Using a Virtual Machine

This is documentation to build the project inside a VM (virtual machine) setup.

## Vagrant setup (WIP)

There is a `VagrantFile` provided, for people that has a working
[Vagrant](https://developer.hashicorp.com/vagrant/docs/installation)
installation:

```sh
cd LampColorControler
vagrant up # turn on the VM +provision +default builds
vagrant ssh # get a shell inside the VM
```

The repository is copied to the VM in `/LampColorControler`, you can do
`vagrant rsync` to synchronize it again, `vagrant reload` to restart the VM,
or `vagrant halt` to stop it.

To build & upload to a lamp plugged as an USB device:

```sh
vagrant ssh -c "cd /LampColorControler && make clean-artifacts upload-indexable"
```

To copy files from the VM guest to the host:

```sh
vagrant plugin install vagrant-scp
mkdir _vagrant_build # must exist before copying
vagrant scp lampbuild:/LampColorControler/<source> ./_vagrant_build/
```

To try X11 forwarding:

```sh
vagrant ssh -c "sudo vim /etc/ssh/sshd_config" # verify sshd config: AddressFamily inet
vagrant ssh -c "sudo systemctl restart sshd"
vagrant ssh -c "cd /LampColorControler && make simulator"
vagrant ssh -c "cd /LampColorControler/_build/simulator && ./indexable-simulator" -- -Y
```

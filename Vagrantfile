Vagrant.configure("2") do |config|
  config.vm.box = "bento/fedora-29"
  config.vm.synced_folder ".", "/home/vagrant/hawkbeans", mount_options: ["dmode=0770", "fmode=0770"]
  config.vm.provision :shell, path: "https://gist.githubusercontent.com/khale/6d21e4c6165725452bf85d42456ae393/raw/afb42cbe0aa7f893129f5277a5fe6c14fa4ccaed/hawkbeans-bootstrap.sh"
  
  config.vm.provider "vmware_desktop" do |v|
    v.gui = true
  end

  config.vm.provider "virtualbox" do |v|
    v.gui = true
  end

end

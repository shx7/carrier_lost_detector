import unittest
import re
import os
from subprocess import check_output


class CarrierLostTests(unittest.TestCase):


    def test_virtual_links_up(self):
        map(lambda x: self.assertTrue(x in up_links), links[1:])


    def test_PCI_not_changed(self):
        pci_id = open(info_dir + 'device/vendor').read()[:-1] + ':' + open(info_dir + 'device/device').read()[:-1]
        self.assertTrue(pci_id in rules)


    def test_MAC_not_changed(self):
        self.assertTrue(open(info_dir + 'address').read()[:-1] in rules)


    def test_ip_not_changed(self):
        for link in links:
            ifcfg_path = '/etc/sysconfig/network/ifcfg-' + link
            if (os.path.isfile(ifcfg_path)):
                ifcfg = open(ifcfg_path).read()
                if ("BOOTPROTO='dhcp'" not in ifcfg):
                    link_addrs = filter(None, re.findall("IPADDR.*='(?P<ip>.*?)'", ifcfg))
                    ip_addr_show_link = check_output('ip addr show ' + link, shell=True)
                    map(lambda x: self.assertTrue(x in ip_addr_show_link), link_addrs)


    def test_routes_not_changed(self):
        routes_path = '/etc/sysconfig/network/routes'
        if (os.path.isfile(routes_path)):
            all_routes = map(lambda x: x.split(' '), open(routes_path).readlines())
            for link in links:
                our_routes = filter(lambda x: x[-2] == link, all_routes)
                ip_route = check_output('ip route', shell=True)
                for route in our_routes:
                    ip, gateway, mask = route[:-2]
                    if mask == '-':
                        self.assertTrue('default via ' + gateway + ' dev ' + link in ip_route)
                    else:
                        if len(mask) > 2:
                            mask = reduce(lambda res, x: res + bin(int(x)).count('1'), mask.split('.'), 0)
                        self.assertTrue(ip + '/' + mask + ' via ' + gateway + ' dev ' + link in ip_route)



if __name__ == '__main__':
    rules = open(re.search("RULES_FILE='(?P<path>.*?)'", open('/lib/udev/write_net_rules').read()).group('path')).read()
    link_names = re.findall('NAME="(?P<name>.*?)"', rules)
    for name in link_names:
        info_dir = '/sys/class/net/' + name + '/'
        up_links = check_output('ip link show up', shell=True)
        if (os.path.isdir(info_dir) and name in up_links and open(info_dir + '/carrier').read(1) == '0'):
            links = [name] + re.findall(': (?P<vdev>.*?)@' + name, check_output('ip link show', shell=True))
            suite = unittest.TestLoader().loadTestsFromTestCase(CarrierLostTests)
            unittest.TextTestRunner(verbosity=2).run(suite)

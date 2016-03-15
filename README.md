# zabbix-pidinfo
Loadable module for zabbix-agent with some functions to obtaining summary information of processes.  

## That is
This plugin add 3 metrics to zabbix-agent:
* procinf.vmrss - returns summary information on the used resident memory of the same name.
* procinf.allmap - returns summary information about the size of a memory block of the same name for the process.
* procinf.rwmap - same as allmap, but counted only the blocks where the process can write and read.
* procinf.shmap - same as allmap, but counted only the shared blocks of the process.  

## Parameters  
This metrics have 2 parameters: process name and username (optional), for example:  
`procinf.vmrss[java,user]`  
All these metrics return the size in bytes.  

## Known problems  
* Plugin may [crash](https://support.zabbix.com/browse/ZBX-8470) zabbix-agent, if redhat/centos used. For fix it, you need update zabbix-agent. 
* To calculate the information plugin processes /proc/pid filesystem, so plugin will not have access to the information of other users of the process. For fix it run the zabbix-agent under the same user as the measured process.
* This is beta-version.

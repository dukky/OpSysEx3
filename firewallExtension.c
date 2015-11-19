#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <net/tcp.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Extensions to the firewall") ;
MODULE_LICENSE("GPL");


/* make IP4-addresses readable */

#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]


struct nf_hook_ops *reg;
struct ruleList *rule_list;

struct ruleList {
       int port;
       char *executable_path;
       struct ruleList *next;
};

void ruleListAdd(struct ruleList *new) {
        struct ruleList *curr;
        struct ruleList *tmp;
        if(rule_list != NULL) {
                curr = rule_list;
                tmp = rule_list->next;
                while(tmp) {
                        curr = tmp;
                        tmp = tmp->next;
                }

                curr->next = new;
        } else {
                // List is empty
                rule_list = new;
        }
}

void ruleListFree(void) {
        struct ruleList *curr;
        struct ruleList *tmp;
        curr = rule_list;
        tmp  = NULL;
        while(curr) {
                tmp = curr->next;
                kfree(curr->executable_path);
                kfree(curr);
                curr = tmp;
        }
}

void ruleListPrint(void) {
        struct ruleList *tmp = rule_list;
        while(tmp) {
                printk(KERN_INFO "Firewall Rule: %d %s\n", tmp->port, tmp->executable_path);
                tmp = tmp->next;
        }
}

int ruleListContains(int port) {
        struct ruleList *tmp = rule_list;
        while(tmp) {
                if(tmp->port == port) {
                        return 1;
                }
                tmp = tmp->next;
        }
        return 0;
}

int ruleListContains(int port, char *path) {
        struct ruleList *tmp = rule_list;
        while(tmp) {
                if((tmp->port == port) && (strcmp(path, tmp->executable_path) == 0)) {
                        return 1;
                }
                tmp = tmp->next;
        }
        return 0;
}


//int ruleListContains

#define BUFFERSIZE 80


unsigned int FirewallExtensionHook (const struct nf_hook_ops *ops,
				    struct sk_buff *skb,
				    const struct net_device *in,
				    const struct net_device *out,
				    int (*okfn)(struct sk_buff *)) {

    struct tcphdr *tcp;
    struct tcphdr _tcph;
    struct sock *sk;

    struct path path;
    pid_t mod_pid;
    struct dentry *procDentry;
    struct dentry *parent;

    char cmdlineFile[BUFFERSIZE];
    int res;

    int port;

    char *result;

    sk = skb->sk;
    if (!sk) {
            printk (KERN_INFO "firewall: netfilter called with empty socket!\n");;
            return NF_ACCEPT;
    }

    if (sk->sk_protocol != IPPROTO_TCP) {
            printk (KERN_INFO "firewall: netfilter called with non-TCP-packet.\n");
            return NF_ACCEPT;
    }



    /* get the tcp-header for the packet */
    tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
    if (!tcp) {
	printk (KERN_INFO "Could not get tcp-header!\n");
	return NF_ACCEPT;
    }
    if (tcp->syn) {
	struct iphdr *ip;

	printk (KERN_INFO "firewall: Starting connection \n");
	ip = ip_hdr (skb);
	if (!ip) {
	    printk (KERN_INFO "firewall: Cannot get IP header!\n!");
	}
	else {
	    printk (KERN_INFO "firewall: Destination address = %u.%u.%u.%u\n", NIPQUAD(ip->daddr));
	}
	printk (KERN_INFO "firewall: destination port = %d\n", ntohs(tcp->dest));

        mod_pid = current->pid;
        snprintf (cmdlineFile, BUFFERSIZE, "/proc/%d/exe", mod_pid);
        res = kern_path (cmdlineFile, LOOKUP_FOLLOW, &path);
        if (res) {
                printk (KERN_INFO "Could not get dentry for %s!\n", cmdlineFile);
                return -EFAULT;
        }

        procDentry = path.dentry;
        printk (KERN_INFO "The name is %s\n", procDentry->d_name.name);
        parent = procDentry->d_parent;
        printk (KERN_INFO "The name of the parent is %s\n", parent->d_name.name);
        result = d_path(&path, cmdlineFile, BUFFERSIZE);
        printk (KERN_INFO "After calling d_path %s\n", result);



	if (in_irq() || in_softirq()) {
		printk (KERN_INFO "Not in user context - retry packet\n");
		return NF_ACCEPT;
	}

        port = ntohs(tcp->dest);
        if(ruleListContains(port)) {
                // There is a rule for this port

        } else {
                // No rule for this port
                return NF_ACCEPT;
        }

	if (ntohs (tcp->dest) == 80) {
	    tcp_done (sk); /* terminate connection immediately */
	    printk (KERN_INFO "Connection shut down\n");
	    return NF_DROP;
	}
    }
    return NF_ACCEPT;
}

EXPORT_SYMBOL (FirewallExtensionHook);

static struct nf_hook_ops firewallExtension_ops = {
	.hook    = FirewallExtensionHook,
	.owner   = THIS_MODULE,
	.pf      = PF_INET,
	.priority = NF_IP_PRI_FIRST,
	.hooknum = NF_INET_LOCAL_OUT
};

int init_module(void)
{

  int errno;
  struct ruleList *first;
  char string[] = "/bin/nc.openbsd";
  char *memstring;

  errno = nf_register_hook (&firewallExtension_ops); /* register the hook */
  if (errno) {
    printk (KERN_INFO "Firewall extension could not be registered!\n");
  }
  else {
    printk(KERN_INFO "Firewall extensions module loaded\n");
  }

  // init the list with dummy data
  first = kmalloc(sizeof(struct ruleList), GFP_KERNEL);

  memstring = kmalloc(sizeof(char) * strlen(string), GFP_KERNEL);
  strcpy(memstring, string);
  first->executable_path = memstring;
  first->port = 80;
  first->next = NULL;
  ruleListAdd(first);
  ruleListPrint();

  // A non 0 return means init_module failed; module can't be loaded.
  return errno;
}


void cleanup_module(void)
{
        ruleListFree();
        nf_unregister_hook (&firewallExtension_ops); /* restore everything to normal */
        printk(KERN_INFO "Firewall extensions module unloaded\n");
}

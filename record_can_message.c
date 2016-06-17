void* receiveCanMsg(void *arg)
{
    int    s = -1;
    struct can_frame frame;
    char *ptr;
    char buf[BUF_SIZ];
    int n = 0, n1 = 0;
    int n2 = 0;
    int nbytes, i, j, k,l;
    uint32_t id, mask;
    FILE * fp;
    FILE * fp2;
    char bin[33];
    char binbu[200];
    char binbu2[200];
    char prop[1];
    char last = '0';

    if( access("/data/canmsg.txt", 0) == 0 )
        remove("/data/canmsg.txt");
    fp = fopen("/data/canmsg.txt", "w");

    if( access("/data/canmsgf.txt", 0) == 0 )
        remove("/data/canmsgf.txt");
    fp2 = fopen("/data/canmsgf.txt", "w");

    signal(SIGPIPE, SIG_IGN);

    can_init();

    if (can_sock(&s) < 0)
        exit (EXIT_FAILURE);

    uint32_t canId = 0;
    while (1) {
        property_get("gm.canmsg.enable", prop, "0");

        if(prop[0] == '1' && last == '0'){
            fclose(fp);
            fclose(fp2);
            remove("/data/canmsg.txt");
            remove("/data/canmsgf.txt");
            fp = fopen("/data/canmsg.txt", "w");
            fp2 = fopen("/data/canmsgf.txt", "w");
        }

        if ((nbytes = read(s, &frame, sizeof(struct can_frame))) < 0) {
            ALOGE("read can socket failed\n");
            return NULL;
        } else {
            if (frame.can_id & CAN_EFF_FLAG)
            {
                n = snprintf(buf, BUF_SIZ, "<0x%08x> ", frame.can_id & CAN_EFF_MASK);
                canId = frame.can_id & CAN_EFF_MASK;

                if(prop[0] == '1') {
                    for(k=0; k<32; k++){
                       bin[k] = ((1<<(31-k))&canId)>>(31-k)  == 0?'0':'1';
                    }
                    strncpy(binbu, bin, 32);
                    n1 = 32;
                    fputc('#', fp);
                    fwrite(buf+1, n-3, 1, fp);
                    fputc('\n', fp);

                    strncpy(binbu2, "[Message", 8);
                    l = snprintf(binbu2+8, 192, "%d", n2);
                    n2++;
                    binbu2[8+l] = ']';
                    fwrite(binbu2, 9+l, 1, fp2);
                    fputc('\n', fp2);

                    fwrite("ID=",3,1,fp2);
                    fwrite(buf+1, n-3, 1, fp2);
                    fputc('\n', fp2);
                }
            }
            else
            {
                n = snprintf(buf, BUF_SIZ, "<0x%03x> ", frame.can_id & CAN_SFF_MASK);
                canId = frame.can_id & CAN_SFF_MASK;

                if(prop[0] == '1') {
                    for(k=0; k<12; k++){
                        bin[k] = ((1<<(11-k))&canId)>>(11-k)  == 0?'0':'1';
                    }
                    strncpy(binbu, bin, 12);
                    n1 = 12;
                    fputc('#', fp);
                    fwrite(buf+1, n-3, 1, fp);
                    fputc('\n', fp);

                    strncpy(binbu2, "[Message", 8);
                    l = snprintf(binbu2+8, 192, "%d", n2);
                    n2++;
                    binbu2[8+l] = ']';
                    fwrite(binbu2,9+l,1,fp2);
                    fputc('\n', fp2);

                    fwrite("ID=",3,1,fp2);
                    fwrite(buf+1, n-3, 1, fp2);
                    fputc('\n', fp2);
                }
            }

            n += snprintf(buf + n, BUF_SIZ - n, "[%d] ", frame.can_dlc);

            if(prop[0] == '1') {
                bin[0] = bin[1] = bin[2] = '0';
                for(k=3; k<7; k++){
                    bin[k] = ((1<<(3-k+3))&frame.can_dlc)>>(3-k+3)  == 0?'0':'1';
                }
                strncpy(binbu+n1, bin, 7);
                n1+=7;

                fwrite("DLC=",4,1,fp2);
                l = snprintf(binbu2, 200, "%d", frame.can_dlc);
                fwrite(binbu2, l , 1, fp2);
                fputc('\n', fp2);

                fwrite("Data=",5,1,fp2);

            }

            for (i = 0; i < frame.can_dlc; i++) {
                n += snprintf(buf + n, BUF_SIZ - n, "%02x ", frame.data[i]);

                if(prop[0] == '1') {
                    for(k=0; k<8; k++){
                    bin[k] = ((1<<(7-k))&frame.data[i])>>(7-k)  == 0?'0':'1';
                    }
                    strncpy(binbu+n1, bin, 8);
                    n1+=8;

                    if (i == frame.can_dlc-1){
                        l = snprintf(binbu2, 200, "%d", frame.data[i]);
                        fwrite(binbu2, l , 1, fp2);
                    } else {
                        l = snprintf(binbu2, 200, "%d, ", frame.data[i]);
                        fwrite(binbu2, l , 1, fp2);
                    }
                }
            }

            if(prop[0] == '1') {
                fputc('\n', fp2);

                if (n1%8 != 0){
                    for (j=0;j<(n1%8);j++){
                        binbu[n1]='0';
                        n1++;
                    }
                }

                for (j=0; j<n1/8; j++){
                    fwrite(binbu+j*8, 8, 1, fp);
                    fputc(' ', fp);
                }

                fputc('\n',fp);
            }

            if (frame.can_id & CAN_RTR_FLAG)
                n += snprintf(buf + n, BUF_SIZ - n, "remote request");

//            ALOGD("%s\n", buf);
            n = 0;

            handle_can_msg(canId, frame);
        }
        last = prop[0];
    }

    fclose(fp);

    fclose(fp2);

    return NULL;
}


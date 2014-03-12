unsigned char hctoc(const char hc) {
    unsigned char c = hc;
    if((c >= '0') && (c <= '9')) c -= '0';
    else if((c >= 'a') && (c <= 'f')) c -= ('a' - 10);
    else if((c >= 'A') && (c <= 'F')) c -= ('A' - 10);
    return c;
}

function fixDateFormatInDolphinGroupTitle(phr) {
    phr = Ts.toUpperFirst(phr);
    phr = phr.replace("kuuta", "kuussa");
    return phr;
}

Ts.setcall("korjaa_päiväysmuotoilu_ryhmän_nimessä", fixDateFormatInDolphinGroupTitle);

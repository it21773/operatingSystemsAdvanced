struct duration {
        int seconds;
};

program SLEEP_PROG{
    version SLEEP_VERS{
        void add(duration)=1;
    }=1;
}=0x31599123;
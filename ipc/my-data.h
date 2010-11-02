enum NRC_MODE {
  NRC_FAST,
  NRC_SLOW,
  NRC_TURBO,
  NRC_REALTIME
};

struct my_data {
  enum NRC_MODE mode;
  char file[255];
};

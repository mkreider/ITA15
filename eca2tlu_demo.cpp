#include <etherbone.h>
#include <tlu.h>
#include <eca.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h> // sleep

using namespace GSI_ECA;
using namespace GSI_TLU;

static const char* program;
#define ALL           0xfff
#define MS_CH_OUT0    7
#define MS_CH_OUT1    6
#define MS_CH_IN0     0

#define SL_CH_IN0     0 
#define CH_LVDS       2

#define EVT_DPULSE     0xdeadbee0ULL
#define EVT_RISE       0xdeadbee1ULL
#define EVT_FALL       0xdeadbee2ULL
#define EVT_PULSE0     0xdeadbee3ULL
#define EVT_PULSE1     0xdeadbee4ULL
#define EVT_PULSEALL   0xdeadbee5ULL


#define PULSELEN0     40
#define PULSELEN1     400

#define OFFS0         0
#define OFFS1         (OFFS0 + 40)
#define OFFS2         (OFFS1 + 200)
#define OFFS3         (OFFS2 + 100) 

#define LVDS_BITS_HI         0
#define LVDS_BITS_LO         12

#define MS_CH0_HI     ((1 << MS_CH_OUT0) << LVDS_BITS_HI)
#define MS_CH0_LO     ((1 << MS_CH_OUT0) << LVDS_BITS_LO)

#define MS_CHX_HI     (ALL << LVDS_BITS_HI)
#define MS_CHX_LO     (ALL << LVDS_BITS_LO)

#define MS_CH1_HI     ((1 << MS_CH_OUT1) << LVDS_BITS_HI)
#define MS_CH1_LO     ((1 << MS_CH_OUT1) << LVDS_BITS_LO)


//clear ECA
//configure ECA
//enable & activate ECA

//verbose: show eca config 

//configure TLU MS
//configure TLU SL

//verbose: show TLU config

//user input

//loop

  //Create ECA event on MS

  //read timestamp MS
  //read timestamp SL

//compare TSs
//calc mean and StdDev
//show result

#define HIGH_NS		100000
#define LOW_NS		100000
#define CPU_DELAY_MS	20
#define EVENTS		16
#define MAX_CHANNELS	32
#define TAG_BIT		0x40000000

static void die(eb_status_t status, const char* what) {
  fprintf(stderr, "%s: %s -- %s\n", program, what, eb_status(status));
  exit(1);
}



static void enable(ECA& eca, bool en) {
  eb_status_t status;
  if ((status = eca.disable(!en)) != EB_OK) {
    die(status, "ECA::disable(false)");
  }
}

static void flush(ECA& eca) { 
    Table table;
    eb_status_t status;
    /* table is empty */
    if ((status = eca.store(table)) != EB_OK)
      die(status, "ECA::clear");
}

static void commit(ECA& eca, Table& table) { 
    eb_status_t status;
    
    if ((status = eca.store(table)) != EB_OK)
      die(status, "ECA::store");
    if ((status = eca.flipTables()) != EB_OK)
      die(status, "ECA::flip");  
}
    
static void activate(ECA& eca, int channel) {
  eb_status_t status;
  
  if ((status = eca.channels[channel].drain(false)) != EB_OK)
      die(status, "ActionChannel::drain(false)");
    if ((status = eca.channels[channel].freeze(false)) != EB_OK)
      die(status, "ActionChannel::freeze(false)");
  
}


static int send(ECA& eca, int stream, Event event, uint64_t time, uint32_t frac) {
    eb_status_t status;
    uint64_t param;
    Tef tef = ((frac & 0x7) << 29);
    
    if (stream == -1) {
      fprintf(stderr, "%s: specify a stream to send with -s\n", program);
      return 1;
    }

    param = 0;
    

    //printf("tef: %08x\n", tef);
    if ((status = eca.streams[stream].send(EventEntry(event, param, tef, time))) != EB_OK)
      die(status, "EventStream::send");
  
  return 0;
}

void setupPulseDemo(ECA& eca) {
  enable(eca, false);
  flush(eca);

  Table table;
  
  //add rules for double pulse __--_____----___
  table.add(TableEntry(EVT_DPULSE, OFFS0, MS_CH0_HI | MS_CH1_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_DPULSE, OFFS1, MS_CH0_LO | MS_CH1_LO, CH_LVDS, 64));
  table.add(TableEntry(EVT_DPULSE, OFFS2, MS_CH0_HI | MS_CH1_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_DPULSE, OFFS3, MS_CH0_LO | MS_CH1_LO, CH_LVDS, 64));
  
  //add rules for single pulses __--__
  table.add(TableEntry(EVT_PULSE0, 0,  MS_CHX_HI , CH_LVDS, 64));
  table.add(TableEntry(EVT_PULSE0, 40/8, MS_CHX_LO , CH_LVDS, 64));
  
  table.add(TableEntry(EVT_PULSE1, 0,  MS_CH0_HI | MS_CH1_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_PULSE1, 40/8, MS_CH0_LO | MS_CH1_LO, CH_LVDS, 64));
  
  table.add(TableEntry(EVT_PULSEALL, 0,  MS_CHX_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_PULSEALL, 40/8, MS_CHX_LO, CH_LVDS, 64));
  
  commit(eca, table);
  
    //dump(eca)
  //commit and arm

  activate(eca, CH_LVDS);
  enable(eca, true);

}

static void ioSetup(Device& device, uint32_t outputchannels) {
  eb_status_t status;
  std::vector<struct sdb_device> devices;
  
  //find IO ctrl and setup ports
  device.sdb_find_by_identity(0x651ULL, 0x4d78adfd, devices);
  if (devices.size() == 0) {
    die(status, "no io controller found");
  }
  status = device.write((eb_address_t)((eb_address_t)devices[0].sdb_component.addr_first +4), EB_BIG_ENDIAN | EB_DATA32, (eb_data_t)outputchannels);
  if (status != EB_OK) die(status, "failed to create cycle"); 
}





int main(int argc, const char** argv) {
  Socket ms_socket, sl_socket;
  Device ms_device, sl_device;
  std::vector<ECA> ms_ecas, sl_ecas;
  std::vector<TLU> ms_tlus, sl_tlus;
  status_t status;
  program = argv[0];


  if (argc != 3) {
    fprintf(stderr, "%s: expecting argument <master device> <slave device>\n", argv[0]);
    return 1;
  }
  
  ms_socket.open();
  if ((status = ms_device.open(ms_socket, argv[1])) != EB_OK) {
    fprintf(stderr, "%s: failed to open master %s: %s\n", argv[0], argv[1], eb_status(status));
    return 1;
  }
  
  ioSetup(ms_device, MS_CH0_HI | MS_CH1_HI); 
  
  /* Find master ECA */
  ECA::probe(ms_device, ms_ecas);
  assert (ms_ecas.size() == 1);
  ECA& ms_eca = ms_ecas[0];
  
  /* Find master TLU */
  TLU::probe(ms_device, ms_tlus);
  assert (ms_tlus.size() == 1);
  TLU& ms_tlu = ms_tlus[0];
  
  
  
  setupPulseDemo(ms_eca);
  
 //set up master tlu
  // Configure master  TLU to record rising edge timestamps //
  ms_tlu.hook(-1, false);
  ms_tlu.set_enable(false); // no interrupts, please
  ms_tlu.clear(-1);
  ms_tlu.listen(-1, true, true, 4); // Listen on all inputs 
  
  sl_socket.open();
  if ((status = sl_device.open(sl_socket, argv[2])) != EB_OK) {
    fprintf(stderr, "%s: failed to open slave %s: %s\n", argv[0], argv[2], eb_status(status));
    return 1;
  }
  
  ioSetup(sl_device, MS_CH0_HI | MS_CH1_HI); 
  
  /* Find slave ECA */
  ECA::probe(sl_device, sl_ecas);
  assert (sl_ecas.size() == 1);
  ECA& sl_eca = sl_ecas[0];
  
  /* Find slave TLU */
  TLU::probe(sl_device, sl_tlus);
  assert (sl_tlus.size() == 1);
  TLU& sl_tlu = sl_tlus[0];
  
  
  
  setupPulseDemo(sl_eca);
  //configure master IO
  
  
  
  // Configure slave  TLU to record rising edge timestamps //
  sl_tlu.hook(-1, false);
  sl_tlu.set_enable(false); // no interrupts, please
  sl_tlu.clear(-1);
  sl_tlu.listen(-1, true, true, 4); // Listen on all inputs 
  
  
  //set up master eca
  //disable and clear
  
  
  
  usleep(1000);
  //send(ms_eca, 0, 0xdeadbee0ULL, 200000000ULL, 0);
  
  

  uint64_t exectime = 0;
  uint64_t lasttime = 0;
//                  s  m  u  n  
  uint64_t margin = 1000000000ULL;
  uint64_t period =   10000000ULL;
  uint64_t sample = period/5;
  
  std::vector<std::vector<uint64_t> > ms_queues;
  std::vector<std::vector<uint64_t> > sl_queues;
/*  
  //sl_tlu.test(0, 0x7f);
  sl_tlu.pop_all( sl_queues);
  printf("SL TLU: %u\n", sl_queues.size());
    
  for (unsigned i = 0; i < sl_queues.size(); ++i) {
      for (unsigned j = 0; j < sl_queues[i].size(); ++j) {
        printf("slave %d saw an event! %s\n", i, sl_eca.date(sl_queues[i][j]/8).c_str());
      }
    }  
*/
  
  
  for(int i=0; i<1000;i++){
    ms_eca.refresh();
    //round up to multiple of n, x + n-1 div n * n
    exectime = ((ms_eca.time + margin/8 + period/8 -1) / (period/8)) * (period/8);
    if(exectime != lasttime) {
      send(ms_eca, 0, EVT_PULSEALL, exectime, 0);
      send(sl_eca, 0, EVT_PULSEALL, exectime, 0);
    }
    ms_tlu.pop_all( ms_queues);
    sl_tlu.pop_all( sl_queues);
    
    lasttime = exectime;
    usleep(sample/1000);
  }
  
  for(int i=0; i<1000;i++){
    
    ms_tlu.pop_all( ms_queues);
    sl_tlu.pop_all( sl_queues);

    usleep(sample/1000);
  }
  
  
  printf("MS TLU: %u\n", ms_queues.size());
  
  double diffsum, diffsum1 = 0.0;
  double diffmean;
  double diffmin = 0.0;
  double diffmax;
  unsigned int maxsize = 0;

  

 // for (unsigned i = 0; i < sl_queues.size(); ++i) {
      diffsum = 0.0;
      diffmin = 0.0;
      diffmax = 0.0;
      maxsize = ms_queues[4].size();
      for (unsigned j = 0; j < ms_queues[4].size(); ++j) {
        int64_t t = ms_queues[10][j] - ms_queues[4][j];
        //printf("Diff: %d, ch%u v%u \n", t, i, j);
        diffsum += (double)t;
        if (diffmin > (double)t) diffmin = (double)t;
        if (diffmax < (double)t) diffmax = (double)t;
      }
      diffmean = diffsum / (double)maxsize;
      printf("Diff Mean Slave Ch 10-4, %f, %u values captured and compared\n", diffmean, maxsize);
   // }
   
   diffsum = 0.0;
      diffmin = 0.0;
      diffmax = 0.0;
      maxsize = sl_queues[4].size();
      for (unsigned j = 0; j < sl_queues[4].size(); ++j) {
        int64_t t = sl_queues[10][j] - sl_queues[4][j];
        //printf("Diff: %d, ch%u v%u \n", t, i, j);
        diffsum += (double)t;
        if (diffmin > (double)t) diffmin = (double)t;
        if (diffmax < (double)t) diffmax = (double)t;
      }
      diffmean = diffsum / (double)maxsize;
      printf("Diff Mean Slave Ch 10-4, %f, %u values captured and compared\n", diffmean, maxsize);

/*    
        diffsum = 0.0;
   
   int ch = 4;     
        
      if (maxsize < ms_queues[ch].size()) maxsize = ms_queues[ch].size();
      for (unsigned j = 0; j < ms_queues[ch].size(); ++j) {
        time_t t = sl_queues[ch][j] - ms_queues[ch][j];
        printf("Diff: %ull, %u \n", t, j);
        diffsum += (double)t;
        if (diffmin > (double)t) diffmin = (double)t;
        if (diffmax < (double)t) diffmax = (double)t;
      }
  */   
  
  /*
  for (unsigned i = 0; i < ms_queues.size(); ++i) {
      for (unsigned j = 0; j < ms_queues[i].size(); ++j) {
        printf("Master %d saw an event! %s\n", i, ms_eca.date(ms_queues[i][j]/8).c_str());
      }
    }
  
  printf("SL TLU: %u\n", sl_queues.size());
    
  for (unsigned i = 0; i < sl_queues.size(); ++i) {
      for (unsigned j = 0; j < sl_queues[i].size(); ++j) {
        printf("slave %d saw an event! %s\n", i, sl_eca.date(sl_queues[i][j]/8).c_str());
      }
    }  
*/

  return 0;
}




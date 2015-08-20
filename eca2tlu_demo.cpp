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
#define MS_CH_OUT0    7
#define MS_CH_OUT1    6
#define MS_CH_IN0     6

#define SL_CH_IN0     1 
#define CH_LVDS       2

#define EVT_DPULSE     0xdeadbee0ULL
#define EVT_RISE       0xdeadbee1ULL
#define EVT_FALL       0xdeadbee2ULL
#define EVT_PULSE0     0xdeadbee3ULL
#define EVT_PULSE1     0xdeadbee4ULL

#define OFFS0         0
#define OFFS1         (OFFS0 + 40)
#define OFFS2         (OFFS1 + 200)
#define OFFS3         (OFFS2 + 100) 

#define LVDS_BITS_HI         0
#define LVDS_BITS_LO         12

#define MS_CH0_HI     ((1 << MS_CH_OUT0) << LVDS_BITS_HI)
#define MS_CH0_LO     ((1 << MS_CH_OUT0) << LVDS_BITS_LO)

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


static int send(ECA& eca, int stream, Event event, uint64_t off, Tef tef) {
    eb_status_t status;
    uint64_t param;
    uint64_t time;
    
    if (stream == -1) {
      fprintf(stderr, "%s: specify a stream to send with -s\n", program);
      return 1;
    }

    param = 0;
    
    time = eca.time + off;
    printf("TimeB4: %ull, after: %ull\n", eca.time, time);
    if ((status = eca.streams[stream].send(EventEntry(event, param, tef, time))) != EB_OK)
      die(status, "EventStream::send");
  
  return 0;
}


int main(int argc, const char** argv) {
  Socket ms_socket, sl_socket;
  Device ms_device, sl_device;
  status_t status;
  program = argv[0];


  if (argc != 2) {
    fprintf(stderr, "%s: expecting argument <device>\n", argv[0]);
    return 1;
  }
  
  ms_socket.open();
  if ((status = ms_device.open(ms_socket, argv[1])) != EB_OK) {
    fprintf(stderr, "%s: failed to open master %s: %s\n", argv[0], argv[1], eb_status(status));
    return 1;
  }
  
  /* Find master ECA */
  std::vector<ECA> ecas;
  ECA::probe(ms_device, ecas);
  assert (ecas.size() == 1);
  ECA& ms_eca = ecas[0];
  
  /* Find master TLU */
  std::vector<TLU> tlus;
  TLU::probe(ms_device, tlus);
  assert (tlus.size() == 1);
  TLU& ms_tlu = tlus[0];
  
  
  //configure master IO
  
  //set up master eca
  //disable and clear
  enable(ms_eca, false);
  flush(ms_eca);

   ECA& eca = ecas[0];

  Table table;
  
  //add rules for double pulse __--_____----___
  table.add(TableEntry(EVT_DPULSE, OFFS0, MS_CH0_HI | MS_CH1_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_DPULSE, OFFS1, MS_CH0_LO | MS_CH1_LO, CH_LVDS, 64));
  table.add(TableEntry(EVT_DPULSE, OFFS2, MS_CH0_HI | MS_CH1_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_DPULSE, OFFS3, MS_CH0_LO | MS_CH1_LO, CH_LVDS, 64));
  
  //add rules for single pulses __--__
  table.add(TableEntry(EVT_PULSE0, 0,  MS_CH0_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_PULSE0, 40/8, MS_CH0_LO, CH_LVDS, 64));
  
  table.add(TableEntry(EVT_PULSE1, 0,  MS_CH0_HI | MS_CH1_HI, CH_LVDS, 64));
  table.add(TableEntry(EVT_PULSE1, 40/8, MS_CH1_LO, CH_LVDS, 64));
  
  commit(ms_eca, table);
  
    //dump(ms_eca)
  //commit and arm

  activate(ms_eca, CH_LVDS);
  enable(ms_eca, true);
  
  usleep(100000);
  //send(ms_eca, 0, 0xdeadbee0ULL, 200000000ULL, 0);
  
  send(ms_eca, 0, EVT_PULSE0, 2000000000ULL/8, 0);
  send(ms_eca, 0, EVT_PULSE1, 2000000008ULL/8, 4);
/*    
  //set up master tlu
  // Configure master  TLU to record rising edge timestamps //
  tlu.hook(-1, false);
  tlu.set_enable(false); // no interrupts, please
  tlu.clear(-1);
  tlu.listen(-1, true, true, 8); // Listen on all inputs 
  
  socket.open();
  if ((status = sl_device.open(socket, argv[1])) != EB_OK) {
    fprintf(stderr, "%s: failed to open slave %s: %s\n", argv[0], argv[1], eb_status(status));
    return 1;
  }
  
  // Find master TLU //
  TLU::probe(device, tlus);
  assert (tlus.size() == 1);
  TLU& sl_tlu = tlus[0];
  
  
  
  
  

  
  while (1) { 
    std::vector<std::vector<uint64_t> > queues;
    tlu.pop_all(queues);
    for (unsigned i = 0; i < queues.size(); ++i) {
      for (unsigned j = 0; j < queues[i].size(); ++j) {
        printf("Input %d saw an event! %s\n", i, eca.date(queues[i][j]/8).c_str());
      }
    }
    usleep(20000); // 50Hz is enough
  }
*/  
  return 0;
}





  

  //set up two board pulse->tlu demo


  
  
  
  //set up slave tlu


  
    



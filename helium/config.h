#ifndef HEL_CONFIG_H
#define HEL_CONFIG_H

// Configuration of Helium Library


// The DataUnpack queue size specifies how many incoming
// msgpack messages can be received but not processed.
#define DPQ_SIZE         (5)

// The MAX_DATA_LEN specifies the size of the input buffer for
// received SPI messages.  It allocates this much RAM for the input
// buffer.  The buffer must be at least as large as the largest
// message you will receive, plus the size of SpiFrameHdr.
#define MAX_DATA_LEN     128

// When a DataPack object is created, it creates a buffer that will
// hold the msgpack packed data.  The DataPack constructor has an
// optional parameter 'size' that specifies how much memory to
// allocate for this buffer.  If you do not specify the size
// parameter, the constructor will allocate DEFAULT_DP_SIZE bytes.
#define DEFAULT_DP_SIZE  100

#endif

# video-splitter
Split a video file into frames.

A introductory inpsection into video file packets.

Will save all individual frames of a video file to the fs. 

The plan is to convert all video files into a baseline fps to reduce strain on bandwith when sending video files over network.

## usage
**Warning** Do not use a large video file! The frames will be saved to your filesystem.

```
make
./video-splitter video-file.mp4
```

## output
The resulting frames will be in assets/ directory.


TUNE_SRC:=fan_control.cpp data_collector.cpp

all: tune 
tune: tune.cpp $(TUNE_SRC)
	clang++ -ggdb -Wall tune.cpp $(TUNE_SRC) -o tune

clean:
	rm -f tune

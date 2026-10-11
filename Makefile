CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS  = -lws2_32
SRC      = main.cpp \
           Core/BloodBankSystem.cpp Core/Admin.cpp \
           Models/User.cpp Models/Donor.cpp Models/Donation.cpp Models/BloodRequest.cpp \
           Algorithms/MergeSort.cpp Algorithms/Search.cpp \
           Database/FileManager.cpp \
           Server/HttpServer.cpp
TARGET   = KUET_BloodBank.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

web: $(TARGET)
	./$(TARGET) --web

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

#include "cesium.h"
#include <common/types.h>
#include <execinfo.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include <mpi.h>
#include "mpijob.h"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <util/assert.h>
#include <util/matlab.h>

using slib::cesium::Cesium;
using slib::cesium::JobController;
using slib::cesium::JobDescription;
using slib::cesium::JobNode;
using slib::cesium::JobOutput;
using slib::cesium::VariableType;
using slib::util::MatlabMatrix;
using std::map;
using std::string;
using std::stringstream;

DEFINE_bool(use_handler, false, "If true, will use the error handler below.");
DEFINE_bool(wait, false, "If true, will wait before doing anything so that you can debug.");

int rank, size;

void TestFunction(const JobDescription& job, JobOutput* output) {
  if (rank == 2) {
    raise(SIGSEGV);
  }

  output->indices.push_back(job.indices[0]);
}

void handler(int sig) {
  void *array[10];
  size_t size;
  
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);
  
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  // Tell the master we're "done"
  JobOutput died;
  died.command = CESIUM_NODE_DIED_JOB_STRING;
  JobNode::SendCompletionMessage(MPI_ROOT_NODE);
  JobNode::WaitForCompletionResponse(MPI_ROOT_NODE);
  JobNode::SendJobDataToNode(died, MPI_ROOT_NODE);

  MPI_Finalize();
  exit(0);
}

void HandleError(const int& error_code, const int& node) {
  LOG(INFO) << "There was an error communicating with node: " << node;
}

namespace slib {
  namespace cesium {
    class TestCesiumCommunication {
    public:
      void Run() {
	Cesium* instance = Cesium::GetInstance();
	if (instance->Start() == slib::cesium::CesiumMasterNode) {
	  // Inject error by tricking the framework into thinking
	  // there are more nodes than there are.
	  instance->_size = instance->_size + 1;
	  FLAGS_logtostderr = true;
	  
	  JobDescription job;
	  job.command = "TestFunction";
	  job.variables["variable"] = MatlabMatrix("1");
	  job.indices.push_back(0);
	  job.indices.push_back(1);
	  job.indices.push_back(2);
	  
	  JobOutput output;
	  instance->ExecuteJob(job, &output);
	} 
	instance->Finish();
      }
    };
  }
}

int main(int argc, char** argv) {
  signal(SIGSEGV, handler);   // install our handler

  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  LOG(INFO) << "Processor " << rank << " reporting for duty";

  if (rank == 0) {
    if (FLAGS_wait) {
      string dummy;
      std::cin >> dummy;
    }

    JobDescription job;
    job.command = "TEST COMMAND";
    job.indices.push_back(1);

    JobController controller;
    if (FLAGS_use_handler) {
      controller.SetCommunicationErrorHandler(&HandleError);
    }

    // Purposefully cause a communication error.
    controller.StartJobOnNode(job, size);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  CESIUM_REGISTER_COMMAND(TestFunction);
  slib::cesium::TestCesiumCommunication tester;
  tester.Run();

  MPI_Finalize();

  LOG(INFO) << "ALL TESTS PASSED";
  
  return 0;
}

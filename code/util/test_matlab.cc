#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "matlab.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

DEFINE_int32(iterations, 1, "Iterations to test memory usage");

using Eigen::MatrixXf;
using slib::util::MatlabMatrix;
using std::string;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  for (int i = 0; i < FLAGS_iterations; i++) {
    MatlabMatrix matrix = MatlabMatrix::LoadFromFile("./test.mat");
    MatlabMatrix field1 = matrix.GetStructField("field1");
    MatlabMatrix field2 = matrix.GetStructField("field2");
    MatlabMatrix field3 = matrix.GetStructField("field3");
    
    MatlabMatrix subfield1 = field1.GetStructField("subfield1");
    MatlabMatrix subfield2 = field1.GetStructField("subfield2");
    
    MatlabMatrix subfield2_cell1 = subfield2.GetCell(0, 0);
    MatlabMatrix subfield2_cell2 = subfield2.GetCell(1, 0);
    
    MatlabMatrix field2_cell1 = field2.GetCell(0, 0);
    MatlabMatrix field2_cell2 = field2.GetCell(0, 1);
    MatlabMatrix field2_cell3 = field2.GetCell(1, 0);
    MatlabMatrix field2_cell4 = field2.GetCell(1, 1);
    
    if (FLAGS_iterations == 1) {
      LOG(INFO) << "\n" << subfield1.GetContents();
      LOG(INFO) << "\n" << field3.GetContents();
      
      LOG(INFO) << "\n" << subfield2_cell1.GetContents();
      LOG(INFO) << "\n" << subfield2_cell2.GetContents();
      
      LOG(INFO) << "\n" << field2_cell1.GetContents();
      LOG(INFO) << "\n" << field2_cell2.GetContents();
      LOG(INFO) << "\n" << field2_cell3.GetContents();
      LOG(INFO) << "\n" << field2_cell4.GetContents();
    }
    FloatMatrix m(2,2);
    m << 1, 2, 3, 4;
    
    subfield1.SetContents(m);
    field1.SetStructField("subfield1", subfield1);
    matrix.SetStructField("field1", field1);
    
    matrix.SaveToFile("./test_c.mat");
    
    string serialized = matrix.Serialize();
    MatlabMatrix deserialized_matrix;
    deserialized_matrix.Deserialize(serialized);
    deserialized_matrix.SaveToFile("./test_c_serialized.mat");

    FloatMatrix R1 = MatrixXf::Random(3, 3);
    FloatMatrix R2 = MatrixXf::Random(3, 3);
    FloatMatrix R3 = MatrixXf::Random(3, 3);

    MatlabMatrix cell1(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 1));
    MatlabMatrix cell2(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 2));
    MatlabMatrix cell3(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 3));

    cell1.SetCell(0, MatlabMatrix(R1));
    cell2.SetCell(1, MatlabMatrix(R2));
    cell3.SetCell(2, MatlabMatrix(R3));

    MatlabMatrix cells(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 1));
    cells.Merge(cell1).Merge(cell2).Merge(cell3);

    if (FLAGS_iterations == 1) {
      LOG(INFO) << "\nCell 1:\n" << cell1.GetCell(0).GetContents();
      LOG(INFO) << "\nCell 2:\n" << cell2.GetCell(1).GetContents();
      LOG(INFO) << "\nCell 3:\n" << cell3.GetCell(2).GetContents();
      LOG(INFO) << "\nCell 1:\n" << cells.GetCell(0).GetContents();
      LOG(INFO) << "\nCell 2:\n" << cells.GetCell(1).GetContents();
      LOG(INFO) << "\nCell 3:\n" << cells.GetCell(2).GetContents();
    }

    MatlabMatrix cells1(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 4));
    cells1.SetCell(0, MatlabMatrix(R1));
    cells1.SetCell(2, MatlabMatrix(R2));
    cells1.SetCell(3, MatlabMatrix("hello from #4"));

    const string serialized_cells1 = cells1.Serialize();
    MatlabMatrix deserialized_cells1;
    deserialized_cells1.Deserialize(serialized_cells1);
    
    if (FLAGS_iterations == 1) {
      LOG(INFO) << "====================================";
      LOG(INFO) << "\nCell 1:\n" << cells1.GetCell(0).GetContents();
      LOG(INFO) << "\nCell 2:\n" << cells1.GetCell(1).GetContents();
      LOG(INFO) << "\nCell 3:\n" << cells1.GetCell(2).GetContents();
      LOG(INFO) << "\nCell 4:\n" << cells1.GetCell(3).GetStringContents();
      
      LOG(INFO) << "\nCell 1:\n" << deserialized_cells1.GetCell(0).GetContents();
      LOG(INFO) << "\nCell 2:\n" << deserialized_cells1.GetCell(1).GetContents();
      LOG(INFO) << "\nCell 3:\n" << deserialized_cells1.GetCell(2).GetContents();
      LOG(INFO) << "\nCell 4:\n" << deserialized_cells1.GetCell(3).GetStringContents();
    }

  }
  
  return 0;
}
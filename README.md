# Model Interpolator and Evaluator.

This Program, given a dataset (an CTL file), interpolates it and evaluates the model.

## Usage

```shell
Usage: MIE -f [Original] [Interpolated] [OPTIONS]
Options:
  -h, --help                   | This is the flag to show the help message
  -f [Original] [Interpolated] | The files that we want to compare (required)
  -s [Seed]                    | The seed for the random number generator (default: random value)
  -o [Output File]             | The output file where the results will be saved (default: standard output)
  -p [%]                       | The percentage of the data that will be used for training (default: 2%)
  -r [Runs]                    | The number of runs (default: 1)
```

## Outputs

The output will be two CSV files, one with the interpolated data and the other with the evaluation of the model.

### Interpolated Data

The interpolated data will be saved in a CSV file with the following format:

```csv
run, i, real, predicted, error
1, 36936, 52.541401, 85.738976, 33.197575
1, 24812, 457.049988, 354.981232, -102.068756
1, 252457, 39.055367, 44.811356, 5.755989
1, 372484, 245.524994, 293.775116, 48.250122
1, 167318, 72.787498, 106.616310, 33.828812
1, 5330, 1.385294, 3.716497, 2.331203
1, 259779, 1.785714, 6.517156, 4.731441
...
```

### Model Evaluation

The model evaluation will be saved in a CSV file with the following format:

```csv
run, RMSE, MAE, MSE
1, 13.880242, 2.993167, 192.661118
...
95, 14.259590, 3.031706, 203.335922
96, 14.263112, 3.034593, 203.436356
97, 14.407369, 3.033309, 207.572281
98, 13.973667, 3.013239, 195.263367
99, 14.303950, 3.038422, 204.602982
100, 14.122334, 3.017058, 199.440323
Average, 14.319799, 3.035945, 205.116470
```

## Code

The program uses the 'c_ctl' library to read the CTL files and the 'compose' program to interpolate the data.

You can find the code in the following repositories:
  - [c_ctl](i forgor 💀)
  - [compose](i forgor 💀)
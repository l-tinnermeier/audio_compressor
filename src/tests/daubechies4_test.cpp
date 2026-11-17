#include <vector>
#include <utility>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>

using namespace std;
  using std::vector;
  using std::max;
  using std::size_t;
  using std::cerr;

vector<pair<double, double>> daubechies4Encode(vector<double>& input) {
  if (input.size() < 2 || input.size() % 2 != 0) {
    throw invalid_argument("Input must contain a positive, even number of samples");
  }

  const size_t half = input.size() / 2;
  const double sqrt3 = sqrt(3.0);
  const double scale = 1.0 / (4.0 * sqrt(2.0));

  const double a = (sqrt3 + 1.0) * scale;
  const double b = (sqrt3 + 3.0) * scale;
  const double c = (3.0 - sqrt3) * scale;
  const double e = (1.0 - sqrt3) * scale;

  vector<pair<double, double>> result(half);

  for (size_t i = 0; i < half; i++) {
    size_t next = (i + 1) % half;
    size_t previous = (i == 0) ? half - 1 : i - 1;

    const double approx = 
      a * input[2 * i] +
      b * input[2 * i + 1] +
      c * input[2 * next] +
      e * input[2 * next + 1];

    const double detail = 
      e * input[2 * previous] -
      c * input[2 * previous + 1] +
      b * input[2 * i] -
      a * input[2 * i + 1];

    result[i] = {approx, detail};
  }
  return result;
}

vector<double> daubechies4Decode(const vector<pair<double, double>>& coefficients) {
  if (coefficients.empty()) { throw invalid_argument("D4 coefficients must not be empty"); }

  const size_t half = coefficients.size();
  const double sqrt3 = sqrt(3.0);
  const double scale = 1.0 / (4.0 * sqrt(2.0));

  const double a = (sqrt3 + 1.0) * scale;
  const double b = (sqrt3 + 3.0) * scale;
  const double c = (3.0 - sqrt3) * scale;
  const double e = (1.0 - sqrt3) * scale;

  vector<double> result(2 * half, 0.0);

  for (size_t i = 0; i < half; i++) {
    size_t next = (i + 1) % half;
    size_t previous = (i == 0) ? half - 1 : i - 1;

    const double approx = coefficients[i].first;
    const double detail = coefficients[i].second;

    // reverse approx
    result[2 * i] += a * approx;
    result[2 * i + 1] += b * approx;
    result[2 * next] += c * approx;
    result[2 * next + 1] += e * approx;

    // reverse detail
    result[2 * previous] += e * detail;
    result[2 * previous + 1] += -c * detail;
    result[2 * i] += b * detail;
    result[2 * i + 1] += -a * detail;

  }
  return result;
}

int main() {
  
  vector<double> test{1, 2, 3, 4, 5, 6, 7, 8};
  const auto coefficients = daubechies4Encode(test);
  const auto restored = daubechies4Decode(coefficients);

  if (restored.size() != test.size()) {
    cerr << "Incorrect reconstructed length" << endl;
    return 1;
  }

  for (size_t i = 0; i < coefficients.size(); i++) {
    cout << "Pair " << i << ": approximation = " << coefficients[i].first << ", detail = " << coefficients[i].second << endl; 
  }

  double maxErr = 0.0;

  for (size_t i = 0; i < test.size(); i++) {
    cout << restored[i] << ' ';
    maxErr = max(maxErr, abs(test[i] - restored[i]));
  }

  cout << "Max error: " << maxErr << endl;

  return maxErr < 1e-12 ? 0 : 1;
}

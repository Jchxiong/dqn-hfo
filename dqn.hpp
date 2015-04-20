#ifndef DQN_HPP_
#define DQN_HPP_

#include <memory>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <HFO.hpp>
#include <caffe/caffe.hpp>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>

namespace dqn {

constexpr auto kStateDataSize = 58;
constexpr auto kInputCount = 2;
constexpr auto kInputDataSize = kStateDataSize * kInputCount;
constexpr auto kMinibatchSize = 32;
constexpr auto kMinibatchDataSize = kInputDataSize * kMinibatchSize;
constexpr auto kOutputCount = 5;

using StateData = std::array<float, kStateDataSize>;
using StateDataSp = std::shared_ptr<StateData>;
using InputStates = std::array<StateDataSp, kInputCount>;
using Transition = std::tuple<InputStates, int,
                              float, boost::optional<StateDataSp>>;

using StateLayerInputData = std::array<float, kMinibatchDataSize>;
using TargetLayerInputData = std::array<float, kMinibatchSize * kOutputCount>;
using FilterLayerInputData = std::array<float, kMinibatchSize * kOutputCount>;

using ActionValue = std::pair<int, float>;
using SolverSp = std::shared_ptr<caffe::Solver<float>>;
using NetSp = boost::shared_ptr<caffe::Net<float>>;

/**
 * Deep Q-Network
 */
class DQN {
public:
  DQN(const std::vector<int>& legal_actions,
      const caffe::SolverParameter& solver_param,
      const int replay_memory_capacity,
      const double gamma,
      const int clone_frequency) :
        legal_actions_(legal_actions),
        solver_param_(solver_param),
        replay_memory_capacity_(replay_memory_capacity),
        gamma_(gamma),
        clone_frequency_(clone_frequency),
        random_engine(0) {}

  // Initialize DQN. Must be called before calling any other method.
  void Initialize();

  // Load a trained model from a file.
  void LoadTrainedModel(const std::string& model_file);

  // Restore solving from a solver file.
  void RestoreSolver(const std::string& solver_file);

  // Snapshot the current model
  void Snapshot() { solver_->Snapshot(); }

  // Select an action by epsilon-greedy.
  int SelectAction(const InputStates& input_states, double epsilon);

  // Select a batch of actions by epsilon-greedy.
  std::vector<int> SelectActions(const std::vector<InputStates>& states_batch,
                                 double epsilon);

  // Add a transition to replay memory
  void AddTransition(const Transition& transition);

  // Update DQN using one minibatch
  void Update();

  // Clear the replay memory
  void ClearReplayMemory() { replay_memory_.clear(); }

  // Get the current size of the replay memory
  int memory_size() const { return replay_memory_.size(); }

  // Return the current iteration of the solver
  int current_iteration() const { return solver_->iter(); }

protected:
  // Clone the Primary network and store the result in clone_net_
  void ClonePrimaryNet();

  // Given a set of input states and a network, select an
  // action. Returns the action and the estimated Q-Value.
  ActionValue SelectActionGreedily(caffe::Net<float>& net,
                                   const InputStates& last_states);

  // Given a batch of input states, return a batch of selected actions + values.
  std::vector<ActionValue> SelectActionGreedily(
      caffe::Net<float>& net,
      const std::vector<InputStates>& last_states);

  // Input data into the State/Target/Filter layers of the given
  // net. This must be done before forward is called.
  void InputDataIntoLayers(caffe::Net<float>& net,
                           const StateLayerInputData& states_data,
                           const TargetLayerInputData& target_data,
                           const FilterLayerInputData& filter_data);

protected:
  const std::vector<int> legal_actions_;
  const caffe::SolverParameter solver_param_;
  const int replay_memory_capacity_;
  const double gamma_;
  const int clone_frequency_; // How often (steps) the clone_net is updated
  std::deque<Transition> replay_memory_;
  SolverSp solver_;
  NetSp net_; // The primary network used for action selection.
  NetSp clone_net_; // Clone of primary net. Used to generate targets.
  TargetLayerInputData dummy_input_data_;
  std::mt19937 random_engine;
};

} // namespace dqn

#endif /* DQN_HPP_ */

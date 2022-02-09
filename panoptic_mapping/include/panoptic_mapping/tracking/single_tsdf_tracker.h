#ifndef PANOPTIC_MAPPING_TRACKING_SINGLE_TSDF_TRACKER_H_
#define PANOPTIC_MAPPING_TRACKING_SINGLE_TSDF_TRACKER_H_

#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "panoptic_mapping/3rd_party/config_utilities.hpp"
#include "panoptic_mapping/labels/label_handler_base.h"
#include "panoptic_mapping/map/classification/class_layer.h"
#include "panoptic_mapping/map/instance_id.h"
#include "panoptic_mapping/tools/map_renderer.h"
#include "panoptic_mapping/tracking/id_tracker_base.h"

namespace std {
template <>
struct hash<panoptic_mapping::InstanceID> {
  size_t operator()(panoptic_mapping::InstanceID const& id) const noexcept {
    return hash<int>{}(static_cast<int>(id));
  }
};
}  // namespace std

namespace panoptic_mapping {

class TrackingInfo;
class TrackingInfoAggregator;
class SegmentInfo;

/**
 * @brief Allocates a single submap to emulate running a monolithic TSDF grid as
 * map representation. Combine this module with the SingleTsdfIntegrator.
 */
class SingleTSDFTracker : public IDTrackerBase {
 public:
  struct Config : public config_utilities::Config<Config> {
    int verbosity = 4;

    // Submap allocation config. Set use class_layer to true to perform label
    // integration.
    Submap::Config submap;

    // Use detectron labels for label tracking
    bool use_detectron = false;

    // Use detectron labels for label+instance tracking
    bool use_detectron_panoptic = false;

    // True: Only match rendered and predicted thing segments that have
    // identical class labels. False: Allow matching of thing segments of
    // different classes.
    bool use_class_for_instance_tracking = true;

    // Count as valid iso-surface points whose projected depth is within this
    // distance in meters of the measured depth. Negative values indicate
    // multiples of the voxel size.
    float depth_tolerance = -1.0;

    // Which tracking metric to compute. Supported are 'IoU' and 'overlap'.
    std::string tracking_metric = "IoU";

    // Accept matches that have at least this value in the computed tracking
    // metric.
    float match_acceptance_threshold = 0.5;

    // Subsample the number of looked up vertices when using
    // 'use_approximate_rendering=false' by this factor squared.
    int rendering_subsampling = 1;

    // Only initialize a new instance for masks with this many pixels
    int min_new_instance_size = 0;

    // Renderer settings. The renderer is only used for visualization purposes.
    MapRenderer::Config renderer;

    Config() { setConfigName("SingleTSDFTracker"); }

   protected:
    void setupParamsAndPrinting() override;
    void checkParams() const override;
  };

  SingleTSDFTracker(const Config& config, std::shared_ptr<Globals> globals);
  ~SingleTSDFTracker() override = default;

  void processInput(SubmapCollection* submaps, InputData* input) override;

 protected:
  // Setup utility.
  void setup(SubmapCollection* submaps);

  void parseDetectronClasses(InputData* input);

  /**
   * @brief Parse detectron panoptic labels
   *
   * Convert detectron ids
   *
   * @param input
   * @return std::map<int, std::pair<int, float>>
   */
  std::map<int, SegmentInfo> parseDetectronPanopticLabels(InputData* input);

  /**
   * @brief Render submap for tracking using approximate rendering algorithm.
   *
   * @param submap
   * @param input
   * @return TrackingInfo
   */
  std::vector<TrackingInfo> renderTrackingInfoApproximate(
      const Submap& submap, const InputData& input);

  /**
   * @brief Renders the vertex map for tracking.
   *
   * Adapted from ProjectiveIDTracker::renderTrackingInfoVertices
   *
   * @param map
   * @param input
   * @return TrackingInfo
   */
  std::vector<TrackingInfo> renderTrackingInfoVertices(const Submap& submap,
                                                       const InputData& input);

  /**
   * @brief Compute consistency-resolved panoptic labels for the given input
   *
   * @param submaps
   * @param input
   * @return TrackingInfoAggregator
   */
  TrackingInfoAggregator computeTrackingData(SubmapCollection* submaps,
                                             InputData* input);

 private:
  // The offset for generating new panoptic instance ids
  // TODO(albanesg): this should be higher than the largest class id
  // since for stuff classes we use the class label as panoptic id
  static constexpr int kInstanceIdOffset_ = 256;

  static config_utilities::Factory::RegistrationRos<
      IDTrackerBase, SingleTSDFTracker, std::shared_ptr<Globals>>
      registration_;
  const Config config_;

  int map_id_;
  bool is_setup_ = false;

  int instance_count_ = 0;

  MapRenderer renderer_;  // The renderer is only used if visualization is on.
  cv::Mat rendered_vis_;  // Store visualization data.
};

}  // namespace panoptic_mapping

#endif  // PANOPTIC_MAPPING_TRACKING_SINGLE_TSDF_TRACKER_H_

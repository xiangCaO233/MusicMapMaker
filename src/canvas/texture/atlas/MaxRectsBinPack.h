#pragma once

#include <memory>
#include <vector>

#include "Rect.h"

class TextureInstace;

class MaxRectsBinPack {
 public:
  /// Instantiates a bin of the given size.
  MaxRectsBinPack(int width, int height, float expandrate);

  /// Specifies the different heuristic rules that can be used when deciding
  /// where to place a new rectangle.
  enum FreeRectChoiceHeuristic {
    RectBestShortSideFit,  ///< -BSSF: Positions the rectangle against the
                           ///< short
    ///< side of a free rectangle into which it fits the
    ///< best.
    RectBestLongSideFit,  ///< -BLSF: Positions the rectangle against the
                          ///< long
    ///< side of a free rectangle into which it fits the
    ///< best.
    RectBestAreaFit,  ///< -BAF: Positions the rectangle into the smallest
                      ///< free
    ///< rect into which it fits.
    RectBottomLeftRule,   ///< -BL: Does the Tetris placement.
    RectContactPointRule  ///< -CP: Choosest the placement where the
                          ///< rectangle
                          ///< touches other rects as much as possible.
  };

  /// Inserts the given list of rectangles in an offline/batch mode, possibly
  /// rotated.
  /// @param rects The list of rectangles to insert. This vector will be
  /// destroyed in the process.
  /// @param dst [out] This list will contain the packed rectangles. The
  /// indices will not correspond to that of rects.
  /// @param method The rectangle placement rule to use when packing.
  void Insert(std::vector<RectSize> &rects, std::vector<Rect> &dst,
              FreeRectChoiceHeuristic method);

  /// Inserts a single rectangle into the bin, possibly rotated.
  Rect Insert(int width, int height, FreeRectChoiceHeuristic method);

  void Insert(std::shared_ptr<TextureInstace> meta,
              FreeRectChoiceHeuristic method);

  /// Computes the ratio of used surface area to the total bin area.
  float Occupancy() const;

 private:
  friend class Texturepool;

  int binWidth;
  int binHeight;
  float binexpandrate;

  std::vector<Rect> usedRectangles;
  std::vector<std::shared_ptr<TextureInstace>> restex;
  std::vector<Rect> freeRectangles;

  void Expand(float rate);

  /// Computes the placement score for placing the given rectangle with the
  /// given method.
  /// @param score1 [out] The primary placement score will be outputted here.
  /// @param score2 [out] The secondary placement score will be outputted
  /// here. This isu sed to break ties.
  /// @return This struct identifies where the rectangle would be placed if it
  /// were placed.
  Rect ScoreRect(int width, int height, FreeRectChoiceHeuristic method,
                 int &score1, int &score2) const;

  /// Places the given rectangle into the bin.
  void PlaceRect(const Rect &node);

  /// Computes the placement score for the -CP variant.
  int ContactPointScoreNode(int x, int y, int width, int height) const;

  Rect FindPositionForNewNodeBottomLeft(int width, int height, int &bestY,
                                        int &bestX) const;

  Rect FindPositionForNewNodeBestShortSideFit(int width, int height,
                                              int &bestShortSideFit,
                                              int &bestLongSideFit) const;

  Rect FindPositionForNewNodeBestLongSideFit(int width, int height,
                                             int &bestShortSideFit,
                                             int &bestLongSideFit) const;

  Rect FindPositionForNewNodeBestAreaFit(int width, int height,
                                         int &bestAreaFit,
                                         int &bestShortSideFit) const;

  Rect FindPositionForNewNodeContactPoint(int width, int height,
                                          int &contactScore) const;

  /// @return True if the free node was split.
  bool SplitFreeNode(Rect freeNode, const Rect &usedNode);

  /// Goes through the free rectangle list and removes any redundant entries.
  void PruneFreeList();
};

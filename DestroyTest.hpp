#pragma once

#include "EngineCommon.hpp"

#include <vector>
#include <cstdlib>

class DestroyTest {
  static std::vector< EntityHandle > liveEntities;
  static void addEntityToTest();
public:
  static void initialize();
  static void update();
};

std::vector< EntityHandle > DestroyTest::liveEntities;

void DestroyTest::addEntityToTest() {
    EntityHandle entity = EntityManager::create();
    TransformManager::set( entity, {} );
    liveEntities.push_back( entity );
}

void DestroyTest::initialize() {
  Debug::write( "Running Entity creation and destruction test...\n" );
  for ( int i = 0; i < 10; ++i ) {
    addEntityToTest();
  }
}

void DestroyTest::update() {
  addEntityToTest();
        
  int ind = rand() % liveEntities.size();
  EntityHandle entity = liveEntities[ ind ];
  EntityManager::destroy( entity );
  liveEntities.erase( liveEntities.begin() + ind );
}

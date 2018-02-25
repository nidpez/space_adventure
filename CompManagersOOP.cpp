#include "EngineCommon.hpp"

Component::Component( Entity& entity ) : entity( entity ) {
  all.push_back( this );
}

void Transform::rotate( float rotation ) {
  PROFILE;
  orientation += rotation;
}

void Transform::rotateAround( Vec2 point, float rotation ) {
  PROFILE;
  orientation += rotation;
  position = rotateVec2( transform.position - point, rotation ) + point;
}

void Transform::translate( Vec2 translation ) {
  PROFILE;
  position += translation;
}

void Transform::scale( Vec2 scale ) {
  PROFILE; 
  scale = scale;
}

void Transform::setPosition( Vec2 position ) {
  this->position = position;
}

void Transform::setOrientation( float orientation ) {
  this->orientation = orientation;
}

Vec2 Transform::getPositon() {
  return position;
}

Vec2 Transform::getScale() {
  return scale;
}

float Transform::getOrientation() {
  return orientation;
}
 
 // FIXME take scale into account
bool Collider::circleCircleCollide( Circle circleA, Circle circleB ) {
  PROFILE;
  float radiiSum = circleA.radius + circleB.radius;
  return sqrMagnitude( circleA.center - circleB.center ) <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool Collider::circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  Vec2 ab = circleB.center - circleA.center;
  normalA = normalized( ab );
  normalB = -normalA;

  float radiiSum = circleA.radius + circleB.radius;
  return sqrMagnitude( ab ) <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool Collider::aaRectCircleCollide( Rect aaRect, Circle circle ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // TODO refactor distance to aaRect function
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  float sqrDistance = 0.0f;
  if ( circle.center.x < aaRect.min.x ) {
    sqrDistance += ( aaRect.min.x - circle.center.x ) * ( aaRect.min.x - circle.center.x );
  }
  if ( circle.center.x > aaRect.max.x ) {
    sqrDistance += ( circle.center.x - aaRect.max.x ) * ( circle.center.x - aaRect.max.x );
  }
  if ( circle.center.y < aaRect.min.y ) {
    sqrDistance += ( aaRect.min.y - circle.center.y ) * ( aaRect.min.y - circle.center.y );
  }
  if ( circle.center.y > aaRect.max.y ) {
    sqrDistance += ( circle.center.y - aaRect.max.y ) * ( circle.center.y - aaRect.max.y );
  }
  return sqrDistance <= circle.radius * circle.radius; 
}

// FIXME take scale into account
bool Collider::aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  normalA = {};
  float x = circle.center.x;
  if ( x < aaRect.min.x ) {
    x = aaRect.min.x;
    normalA.x = -1.0;
  }
  if ( x > aaRect.max.x ) {
    x = aaRect.max.x;
    normalA.x = 1.0;
  }
  float y = circle.center.y;
  if ( y < aaRect.min.y ) {
    y = aaRect.min.y;
    normalA.y = -1.0;
  }
  if ( y > aaRect.max.y ) {
    y = aaRect.max.y;
    normalA.y = 1.0;
  }
  normalA = normalized( normalA );
  Vec2 closestPtInRect = { x, y };
  Vec2 circleToRect = closestPtInRect - circle.center;
  normalB = normalized( circleToRect );

  return sqrMagnitude( circleToRect ) <= circle.radius * circle.radius;
}

// TODO implement
bool Collider::aaRectAARectCollide( Rect aaRectA, Rect aaRectB ) {
  aaRectA = aaRectB = {};
  return true;
}

// TODO implement
bool Collider::aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB ) {
  aaRectA = aaRectB = {};
  normalA = normalB = {};
  return false;
}

void Collider::updateAndCollide() {
  PROFILE;
  // space partitioned collision detection
  // keep the quadtree updated
  // TODO calculate the boundary dynamically 
  Rect boundary = { { -70, -40 }, { 70, 40 } };
  buildQuadTree( all, boundary );
  // detect collisions
  collisions.clear();
  collisions.resize( componentMap.components.size(), {} );
  for ( u32 nodeInd = 1; nodeInd < quadTree.size(); ++nodeInd ) {
    QuadNode quadNode = quadTree[ nodeInd ];
    if ( !quadNode.isLeaf ) {
      continue;
    }
    for ( int i = 0; i < quadNode.elements.lastInd; ++i ) {
      Collider collI = quadNode.elements._[ i ].getInWorldCoords();
      for ( int j = i + 1; j <= quadNode.elements.lastInd; ++j ) {
        Collider collJ = quadNode.elements._[ j ].getInWorldCoords();
        Collision collision;
        if ( collI.collide( collJ, collision ) ) {
          collI.addCollision( collision );
          collJ.addCollision( { collision.b, collision.a, collision.normalB, collision.normalA } );
          Debug::drawShape( collI, Debug::GREEN );
          Debug::drawShape( collJ, Debug::GREEN );
        }
      }
    }
  }
}

bool CircleCollider::collide( Collider colliderB ) {
  return colliderB.collide( *this );
}

bool CircleCollider::collide( Collider colliderB, Collision& collision ) {
  return colliderB.collide( *this, collision );
}

bool CircleCollider::collide( CircleCollider circle ) {
  return Collider::circleCircleCollide( *this, circle );
}

bool CircleCollider::collide( CircleCollider circle, Collision& collision ) {
  collision.a = *this;
  collision.b = circle;
  return Collider::circleCircleCollide( *this, circle, collision.normalA, collision.normalB );
}

bool CircleCollider::collide( AARectCollider aaRect ) {
  return Collider::aaRectCircleCollide( aaRect, *this );
}

bool CircleCollider::collide( AARectCollider aaRect, Collision& collision ) {
  collision.a = *this;
  collision.b = aaRect;
  return Collider::aaRectCircleCollide( aaRect, *this, collision.normalB, collision.normalA );
}

void CircleCollider::fitToSprite( Sprite sprite ) {
  Vec2 size = sprite.getSize();
  float maxSize = ( size.x > size.y ) ? size.x : size.y;
  circle.radius = maxSize / 2.0f;
}

Collider CircleCollider::getInWorldCoords() {
  Transform transform = entity.getTransform();
  float scaleX = transform.getScale().x, scaleY = transform.getScale().y;
  float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
  Vec2 __position = transform.getPosition() + center * maxScale;
  float __radius = radius * maxScale;
  return { entity, position, radius };
}

bool AARectCollider::collide( Collider colliderB ) {
  return colliderB.collide( *this );
}

bool AARectCollider::collide( Collider colliderB, Collision& collision ) {
  return colliderB.collide( *this, collision );
}

bool AARectCollider::collide( CircleCollider circle ) {
  return Collider::aaRectCircleCollide( *this, circle );
}

bool AARectCollider::collide( CircleCollider circle, Collision& collision ) {
  collision.a = *this;
  collision.b = circle;
  return Collider::aaRectCircleCollide( *this, circle, collision.normalA, collision.normalB );
}

bool AARectCollider::collide( AARectCollider aaRect ) {
  return Collider::aaRectAARectCollide( *this, aaRect );
}

bool AARectCollider::collide( AARectCollider aaRect, Collision& collision ) {
  collision.a = *this;
  collision.b = aaRect;
  return Collider::aaRectAARectCollide( *this, aaRect, collision.normalA, collision.normalB );
}

Collider AARectCollider::getInWorldCoords() {
  Transform transform = entity.getTransform();
  Vec2 min = min * transform.getScale() + transform.getPosition();
  Vec2 max = max * transform.getScale() + transform.getPosition();
  return { entity, min, max };
}

void QuadTree::QuadTree( Rect boundary, std::vector< Collider& > colliders ) {
  PROFILE;
  rootNode( {}, boundary );
  for ( u32 colliderInd = 1; colliderInd < colliders.size(); ++colliderInd ) {
    insert( colliders[ colliderInd ] );
  }
  // debug render space partitions
  std::stack< QuadNode& > nodeStack;
  nodeStack.push( rootNode );
  while ( !nodeStack.empty() ) {
    QuadNode& quadNode = nodeStack.top();
    nodeStack.pop();
    if ( !quadNode.isLeaf ) {
      for ( int i = 0; i < 4; ++i ) {
        nodeStack.push( quadNode.children[ i ] );
      }
      continue;
    }
    Debug::drawRect( quadNode.boundary, { 1, 1, 1, 0.3f } );
    for ( int i = 0; i <= quadNode.elements.lastInd; ++i ) {
      Debug::drawShape( quadNode.elements._[ i ].getInWorldCoords(), Debug::BLUE );
    }
  }
}

// FIXME if QuadBucket::CAPACITY + 1 colliders are at the center of the node
//       we will subdivide it forever
void QuadNode::subdivide() {
  PROFILE;
  // backup elements
  QuadBucket __elements = std::move( elements );
  isLeaf = false;
  Vec2 min = boundary.getNin();
  Vec2 max = boundary.getMax();
  Vec2 center = min + ( max - min ) / 2.0f;
  // top-right
  children[ 0 ] = QuadNode( AARect( { center, max } ) );
  // bottom-right
  children[ 1 ] = QuadNode( AARect( { { center.x, min.y }, { max.x, center.y } } ) );
  // bottom-left
  children[ 2 ] = QuadNode( AARect( { min, center } ) );
  // top-left
  children[ 3 ] = QuadNode( AARect( { { min.x, center.y }, { center.x, max.y } } ) );
  // put elements inside children
  for ( int elemInd = 0; elemInd <= __elements.lastInd; ++elemInd ) {
    Collider& collider = __elements._[ elemInd ];
    for ( int childI = 0; childI < 4; ++childI ) {
      QuadNode& child = children[ childI ];
      if ( child.boundary.collide( collider ) ) {
        child.__elements._[ ++child.__elements.lastInd ] = collider;
      }
    }
  }
}

void QuadTree::insert( Collider& collider ) {
  PROFILE;
  std::deque< QuadNode& > nextNodes = std::deque< QuadNode& >();
  if ( collider.collide( rootNode.boundary ) ) {
    nextNodes.push_front( rootNode );
  }
  bool inserted = false;
  while ( !nextNodes.empty() ) {
    QuadNode& node = nextNodes.front();
    nextNodes.pop_front();
    // try to add collider to this node
    if ( node.isLeaf ) {
      // ... if there is still space
      if ( node.elements.lastInd < QuadBucket::CAPACITY - 1 ) {
        node.elements._[ ++node.elements.lastInd ] = collider;
        inserted = true;
      } else { 
        // if there is no space this cannot be a leaf any more
        node.subdivide();
      }
    }
    if ( !node.isLeaf ) {
      // find which children the collider intersects with
      // and add them to the deque
      for ( int i = 0; i < 4; ++i ) {
        QuadNode& child = node.children[ i ];
        if ( collider.collide( child.boundary ) ) {
          nextNodes.push_front( cild );
        }
      }
    }
  }
  ASSERT( inserted, "Collider of entity %d not inserted into QuadTree", &collider.getEntity() ); 
}

void SolidBody::setSpeed( Vec2 speed ) {
  PROFILE;
  this->speed = speed;
}

void SolidBody::update( double deltaT ) {
  PROFILE;
  std::vector< Collision > collisions = entity.getCollider().getCollisions();
  ASSERT( collisions, "Have no collision set for SolidBody %d", this );
  // move solid body
  Vec2 normal = {};
  for ( u32 i = 0; i < collisions.size(); ++i ) {
    if ( dot( speed, collisions[ i ].normalB ) <= 0.0f ) {
      normal += collisions[ i ].normalB;
    }
  }
  if ( normal.x != 0 || normal.y != 0 ) {
    // reflect direction
    normal = normalized( normal );
    float vDotN = dot( speed, normal );
    speed = speed - 2.0f * vDotN * normal;
  }
  entity.getTransform().translate( speed * deltaT );
}

RenderInfo Sprite::renderInfo;
Sprite::Pos* Sprite::posBufferData;
Sprite::UV* Sprite::texCoordsBufferData;

void Sprite::initialize() {
  // configure buffers
  glGenVertexArrays( 1, &renderInfo.vaoId );
  glBindVertexArray( renderInfo.vaoId );
  glGenBuffers( 2, renderInfo.vboIds );
  // positions buffer
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 0 );
  // texture coordinates buffer
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 1 );
  glBindVertexArray( 0 );
  // create shader program
  renderInfo.shaderProgramId = AssetManager::loadShader( "shaders/SpriteUnlit.glsl" );
  // get shader's constants' locations
  renderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.left" );
  renderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.right" );
  renderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.bottom" );
  renderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.top" );
}

void Sprite::Sprite( AssetIndex textureId, Rect texCoords ) : Component( entity ), texCoords( texCoords ) {
  ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId ); 
  this->texCoords = texCoords;
  TextureAsset texture = AssetManager::getTexture( textureId );
  float width = texture.width * ( texCoords.max.u - texCoords.min.u ) / PIXELS_PER_UNIT;
  float height = texture.height * ( texCoords.max.v - texCoords.min.v ) / PIXELS_PER_UNIT;
  size = { width, height };
}

void Sprite::~Sprite() {
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 2, renderInfo.vboIds );
}
    
void Sprite::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
}

void Sprite::updateAndRender() {
  PROFILE;
  if ( all.size() == 0 ) {
    return;
  }
  // build vertex buffer and render for sprites with same texture
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  // TODO don't render every sprite every time
  u32 spritesToRenderCount = all.size();
  // TODO use triangle indices to reduce vertex count
  u32 vertsPerSprite = 6; 
  posBufferData = new Pos[ spritesToRenderCount * vertsPerSprite ];
  texCoordsBufferData = new UV[ spritesToRenderCount * vertsPerSprite ];
  // build the positions buffer
  Vec2 baseGeometry[] = {
    { -0.5f,	-0.5f },
    { 0.5f,	 0.5f },
    { -0.5f,	 0.5f },
    { -0.5f,	-0.5f },
    { 0.5f,	-0.5f },
    { 0.5f,	 0.5f }
  };
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    Sprite sprite = all[ spriteInd ];
    Transform transform = sprite.getEntity().getTransform();
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      Vec2 vert = baseGeometry[ vertInd ] * sprite.size * transform.scale;
      vert = rotateVec2( vert, transform.orientation );
      vert += transform.position;
      posBufferData[ spriteInd * vertsPerSprite + vertInd ].pos = vert;
    }
  } 
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( Pos ), posBufferData, GL_STATIC_DRAW );
  // TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  // build the texture coordinates buffer
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    Sprite sprite = all[ spriteInd ];
    Vec2 max = sprite.getTextureCoords().getMax();
    Vec2 min = sprite.getTextureCoords().getMin();
    Vec2 texCoords[] = {
      min, max, { min.u, max.v },
      min, { max.u, min.v }, max
    };
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      texCoordsBufferData[ spriteInd * vertsPerSprite + vertInd ].uv = texCoords[ vertInd ];
    }
  }
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( UV ), texCoordsBufferData, GL_STATIC_DRAW );
  // TODO measure how expensive these allocations and deallocations are!
  delete[] texCoordsBufferData;
  // issue render commands
  // TODO keep sorted by texture id
  u32 currentTexId = componentMap.components[ 0 ].sprite.textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  u32 currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  // mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  u32 currentSubBufferStart = 0;
  for ( u32 spriteInd = 1; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( all[ spriteInd ].textureId != currentTexId ) {
      // send current vertex sub-buffer and render it
      u32 spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
      glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
      // and start a new one
      currentTexId = all[ spriteInd ].textureId;      
      ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );
      currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
      glBindTexture( GL_TEXTURE_2D, currentTexGlId );
      currentSubBufferStart = spriteInd;
    }
  }
  // render the last sub-buffer
  u32 spriteCountInSubBuffer = spritesToRenderCount - currentSubBufferStart;
  glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
}

#include "EngineCommon.hpp"

#include <ctime>
#include <cstdarg>

/////////////////////// Error handling and logging //////////////////////

FILE* Debug::log;
RenderInfo Debug::renderInfo;
std::vector< Debug::DebugCircle > Debug::circleBufferData;

void Debug::initializeLogger() {
#ifndef NDEBUG
  //logging stuff
  log = fopen( LOG_FILE_NAME, "a" );
  write( "Logging system initialized.\n" );
#endif
}

void Debug::initializeRenderer() {
#ifndef NDEBUG
  // configure buffers
  glGenVertexArrays( 1, &renderInfo.vaoId );
  glBindVertexArray( renderInfo.vaoId );
  glGenBuffers( 1, &renderInfo.vboIds[ 0 ] );
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 2 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 3 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 2 );
  glBindVertexArray( 0 );
  // create shader program
  renderInfo.shaderProgramId = AssetManager::loadShader( "shaders/DebugShape.glsl" );
  // get shader's constants' locations
  renderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.left" );
  renderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.right" );
  renderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.bottom" );
  renderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.top" );
#endif
}

void Debug::write( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  write( format, args );
  va_end( args );
}

void Debug::write( const char* format, va_list args ) {
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  va_end( args2 );
}

void Debug::writeError( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  writeError( format, args );
  va_end( args );
}

void Debug::writeError( const char* format, va_list args ) {
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  vfprintf( log, format, args2 );
  va_end( args2 );
}
  
void Debug::haltWithMessage( const char* failedCond, const char* file, const char* function, s32 line, ... ) {  
  std::time_t now = std::time( nullptr );
  char* date = std::ctime( &now );
  Debug::writeError( "%s\tAssertion \"%s\" failed\n\tat %s,\n\t\t%s, line %d:\n\t\t",
		      date, failedCond, file, function, line ); 
  va_list args;
  va_start( args, line );
  char* msgFormat = va_arg( args, char* );
  Debug::writeError( msgFormat, args );
  va_end( args );
  Debug::writeError( "\n" );
  Debug::shutdown();
  std::abort();
}

////////////////////////// Drawing debug shapes ///////////////////////////

void Debug::shutdown() {
#ifndef NDEBUG
  // rendering stuff
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 1, &renderInfo.vboIds[ 0 ] );
  // logging stuff
  fclose( log );
#endif
}

void Debug::drawCircle( Circle circle, Color color ) {
#ifndef NDEBUG
  ASSERT( circle.radius > 0.0f, "Asked to draw a circle of radius %f", circle.radius );
  circleBufferData.push_back( { color, circle.center, circle.radius } );
#endif
}

void Debug::renderAndClear() {
#ifndef NDEBUG
  // configure buffers
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, sizeof( DebugCircle ) * circleBufferData.size(), circleBufferData.data(), GL_STATIC_DRAW );
  glDrawArrays( GL_POINTS, 0, circleBufferData.size() );
  circleBufferData.clear();
#endif
}

void Debug::setOrthoProjection( float aspectRatio, float height ) {
#ifndef NDEBUG
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
#endif
}

////////////////////////// Drawing debug shapes ///////////////////////////

AutoProfile::AutoProfile( const char* name ) {
#ifndef NDEBUG
  this->name = name;
  startNanos = std::chrono::duration_cast< std::chrono::nanoseconds >( std::chrono::high_resolution_clock::now() ).count();
#endif
}

AutoProfile::~AutoProfile() {
#ifndef NDEBUG
  u64 endNanos = std::chrono::duration_cast< std::chrono::nanoseconds >( std::chrono::high_resolution_clock::now() ).count();
  u64 elapsedNanos = endNanos - startNanos;
  ProfileManager::addSample( name, elapsedNanos );
#endif
}

std::unordered_map< char*, ProfileManager::ProfileSample, AreNamesEqual > ProfileManager::inclusiveSamples;
FILE* ProfileManager::profilerLog;
u32 ProfileManager::frameNumber;

bool ProfileManager::AreNamesEqual::operator()( const char* a, const char* b ) {
  return std::strcmp( a, b ) == 0;
}

void ProfileManager::initialize() {
#ifndef NDEBUG
  frameNumber = 0;
  profilerLog = fopen( PROFILER_LOG_FILE_NAME, "a" );
  Debug::write( "Profiler initialized.\n" );
#endif
}

void ProfileManager::shutdown() {
#ifndef NDEBUG
  fclose( profilerLog );
#endif
}

void ProfileManager::addSample( const char* name, u64 elapsedNanos ) {
#ifndef NDEBUG
  auto iter = inclusiveSamples.find( name );
  if ( iter != inclusiveSamples.end() ) {
    ProfileSample sample = iter->second;
    sample.elapsedNanos += elapsedNanos;
    sample.count++;
  } else {
    inclusiveSamples.insert( { name, { elapsedNanos, 1 } } );
  }
#endif
}

void ProfileManager::updateOutputsAndReset() {
#ifndef NDEBUG
  fprintf( profilerLog, "%d\n", frameNumber++ );
  for ( auto iter = inclusiveSamples.begin(); iter != inclusiveSamples.end(); ++iter ) {
    ProfileSample sample = iter->second;
    fprintf( profilerLog, "%s, %d, %d\n", iter->first, sample.count, sample.elapsedNanos );
    iter->second = { -1, -1 };
  }
#endif
}

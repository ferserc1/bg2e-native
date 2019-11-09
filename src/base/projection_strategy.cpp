
#include <bg2e/base/projection_strategy.hpp>

#include <bg2e/math/utils.hpp>

namespace bg2e {
namespace base {

	ProjectionStrategy* ProjectionStrategy::Factory(db::json::Value * jsonData) {
		using namespace db::json;
		ProjectionStrategy * result = nullptr;
		if (jsonData) {
			std::string type = Value::String((*jsonData)["type"]);
			if (type == "PerspectiveProjectionMethod") {
				result = new PerspectiveProjectionStrategy();
			}
			else if (type == "OpticalProjectionMethod") {
				result = new OpticalProjectionStrategy();
			}
			if (result) {
				result->deserialize(jsonData);
			}
		}
		return result;
	}

	ProjectionStrategy::ProjectionStrategy()
		:MatrixStrategy()
		,_near(0.3f)
		,_far(1000.0f)
	{
	}

	ProjectionStrategy::ProjectionStrategy(math::float4x4 * target)
		:MatrixStrategy(target)
		,_near(0.3f)
		,_far(1000.0f)
	{

	}

	ProjectionStrategy::~ProjectionStrategy() {

	}

	PerspectiveProjectionStrategy::PerspectiveProjectionStrategy()
		:ProjectionStrategy()
		, _fov(50.0f)
	{
	}

	PerspectiveProjectionStrategy::PerspectiveProjectionStrategy(math::float4x4 * mat)
		:ProjectionStrategy(mat)
		,_fov(50.0f)
	{
	}

	PerspectiveProjectionStrategy::~PerspectiveProjectionStrategy() {
	}

	ProjectionStrategy* PerspectiveProjectionStrategy::clone() {
		PerspectiveProjectionStrategy * result = new PerspectiveProjectionStrategy();
		result->_near = _near;
		result->_far = _far;
		result->_fov = _fov;
		return result;
	}

	void PerspectiveProjectionStrategy::apply() {
		if (target()) {
			target()->perspective(_fov, viewport().aspectRatio(), near(), far());
		}
	}

	db::json::Value* PerspectiveProjectionStrategy::serialize() {
		db::json::Value* value = new db::json::Value();
		value->setValue("type", "PerspectiveProjectionMethod");
		value->setValue("near", _near);
		value->setValue("far", _far);
		value->setValue("fov", _fov);
		return value;
	}

	void PerspectiveProjectionStrategy::deserialize(db::json::Value* data) {
		using namespace db::json;
		_near = Value::Float((*data)["near"]);
		_far = Value::Float((*data)["far"]);
		_fov = Value::Float((*data)["fov"]);
	}


	OpticalProjectionStrategy::OpticalProjectionStrategy()
		:ProjectionStrategy()
		,_focalLength(50.0f)
		,_frameSize(35.0f)
	{
	}

	OpticalProjectionStrategy::OpticalProjectionStrategy(math::float4x4 * mat)
		:ProjectionStrategy(mat)
		,_focalLength(50.0f)
		,_frameSize(35.0f)
	{
	}

	OpticalProjectionStrategy::~OpticalProjectionStrategy() {
	}

	ProjectionStrategy* OpticalProjectionStrategy::clone() {
		OpticalProjectionStrategy* result = new OpticalProjectionStrategy();
		result->_near = _near;
		result->_far = _far;
		result->_focalLength = _focalLength;
		result->_frameSize = _frameSize;
		return result;
	}

	void OpticalProjectionStrategy::apply() {
		if (target()) {
			float fov = math::degrees(2.0f * std::atanf(_frameSize / (_focalLength * 2.0f)));
			target()->perspective(fov, viewport().aspectRatio(), near(), far());
		}
	}

	db::json::Value* OpticalProjectionStrategy::serialize() {
		db::json::Value* value = new db::json::Value();
		value->setValue("type", "OpticalProjectionMethod");
		value->setValue("focalLength", _focalLength);
		value->setValue("frameSize", _frameSize);
		value->setValue("near", _near);
		value->setValue("far", _far);
		return value;
	}

	void OpticalProjectionStrategy::deserialize(db::json::Value* data) {
		using namespace db::json;
		_focalLength = Value::Float((*data)["focalLength"]);
		_frameSize = Value::Float((*data)["frameSize"]);
		_near = Value::Float((*data)["near"]);
		_far = Value::Float((*data)["far"]);
	}


}
}

#include "instance_data_containers/Error.h"

namespace modio
{
  void Error::initialize(ModioError modio_error)
  {
    this->code = modio_error.code;
    if(modio_error.message)
      this->message = modio_error.message;
    this->errors.resize(modio_error.errors_array_size);
    for(int i = 0; i < modio_error.errors_array_size; i++)
    {
      this->errors[i] = modio_error.errors_array[i];
    }
  }
}

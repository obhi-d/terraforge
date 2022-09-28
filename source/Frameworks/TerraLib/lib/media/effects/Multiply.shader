name     = "@local:...";
category = "@local:category";
brief    = "@local:tooltip";
help     = "@local:desc";
function = "Multiply";

shaders
{
  // Higher to lower priority
  config glsl4
  {
    requires = ["..extensins.."];
    files    = ["Multiply4.comp"];
  }

  config default_config
  {
    files = ["Multiply.comp"];
  };
};

param Factor(type = source, min = -inf, max = inf, default = 1.0);
